#include "MSPauseBackdrop.hpp"

#include "../Theme.hpp"
#include "../ui/MSStyle.hpp"

#include <chrono>
#include <cmath>
#include <set>

using namespace geode::prelude;

namespace
{
  // Taps on each side of the middle, two texels each (linear sampling): a sigma up to about 21 texels
  constexpr int MAX_TAPS = 32;
  // A window being resized: the picture is made again once its size has held this long
  constexpr float SETTLE_SECONDS = 0.2f;

  // Positions straight in clip space, so no matrix is needed and nothing of cocos' projection is touched
  constexpr char const* QUAD_VERTEX = R"(
attribute vec4 a_position;
attribute vec2 a_texCoord;
#ifdef GL_ES
varying mediump vec2 v_texCoord;
#else
varying vec2 v_texCoord;
#endif
void main()
{
  gl_Position = a_position;
  v_texCoord = a_texCoord;
}
)";

  // Half the size: sampled at the corner four texels share, the linear filter averages them
  constexpr char const* COPY_FRAGMENT = R"(
#ifdef GL_ES
precision mediump float;
#endif
varying vec2 v_texCoord;
uniform sampler2D CC_Texture0;
void main()
{
  gl_FragColor = vec4(texture2D(CC_Texture0, v_texCoord).rgb, 1.0);
}
)";

  // One direction of the Gaussian. Each tap reads between two texels, so the linear filter weighs both
  constexpr char const* GAUSS_FRAGMENT = R"(
#ifdef GL_ES
precision mediump float;
#endif
varying vec2 v_texCoord;
uniform sampler2D CC_Texture0;
uniform vec2 u_step;
uniform float u_weights[33];
uniform float u_offsets[33];
uniform int u_count;
void main()
{
  vec3 sum = texture2D(CC_Texture0, v_texCoord).rgb * u_weights[0];
  for (int i = 1; i < 33; i++)
  {
    if (i >= u_count)
      break;
    vec2 offset = u_step * u_offsets[i];
    sum += (texture2D(CC_Texture0, v_texCoord + offset).rgb + texture2D(CC_Texture0, v_texCoord - offset).rgb) *
           u_weights[i];
  }
  gl_FragColor = vec4(sum, 1.0);
}
)";

  struct Programs
  {
    CCGLProgram* copy = nullptr;
    CCGLProgram* gauss = nullptr;
    GLint step = -1;
    GLint weights = -1;
    GLint offsets = -1;
    GLint count = -1;
  };

  // Made once a session, kept for the next pauses; never put in cocos' shader cache
  Programs const* programs()
  {
    static Programs s_programs;
    static bool s_tried = false;
    if (!s_tried)
    {
      s_tried = true;
      auto copy = ms::createProgram(QUAD_VERTEX, COPY_FRAGMENT, "The pause blur's copy shader (a darkening instead)");
      auto gauss = ms::createProgram(QUAD_VERTEX, GAUSS_FRAGMENT, "The pause blur's shader (a darkening instead)");
      if (copy && gauss)
      {
        copy->retain();
        gauss->retain();
        s_programs.copy = copy;
        s_programs.gauss = gauss;
        s_programs.step = gauss->getUniformLocationForName("u_step");
        s_programs.weights = gauss->getUniformLocationForName("u_weights");
        s_programs.offsets = gauss->getUniformLocationForName("u_offsets");
        s_programs.count = gauss->getUniformLocationForName("u_count");
      }
    }
    return s_programs.copy ? &s_programs : nullptr;
  }

  // The discrete Gaussian folded into linear-sampling taps: the middle, then pairs of texels as one read between
  // them. Normalised to 1
  int gaussTaps(float sigma, float* weights, float* offsets)
  {
    auto radius = std::max(1, static_cast<int>(std::ceil(sigma * 3)));
    std::vector<float> discrete(radius + 1);
    float total = 0;
    for (int i = 0; i <= radius; i++)
    {
      discrete[i] = std::exp(-0.5f * i * i / (sigma * sigma));
      total += i == 0 ? discrete[i] : 2 * discrete[i];
    }
    weights[0] = discrete[0] / total;
    offsets[0] = 0;
    auto count = 1;
    for (int i = 1; i <= radius && count < MAX_TAPS + 1; i += 2)
    {
      auto a = discrete[i];
      auto b = i + 1 <= radius ? discrete[i + 1] : 0.f;
      weights[count] = (a + b) / total;
      offsets[count] = (i * a + (i + 1) * b) / (a + b);
      count++;
    }
    return count;
  }

  double mebibytes(double bytes) { return bytes / (1024.0 * 1024.0); }

  // Each failure once a session
  void logOnce(std::string const& message)
  {
    static std::set<std::string> s_logged;
    if (s_logged.insert(message).second)
      log::warn("Pause blur: {} (a darkening instead)", message);
  }
} // namespace

MSPauseBackdrop* MSPauseBackdrop::create(CCNode* pause)
{
  auto node = new MSPauseBackdrop();
  if (node->init(pause))
  {
    node->autorelease();
    return node;
  }
  delete node;
  return nullptr;
}

MSPauseBackdrop::~MSPauseBackdrop() { this->freeTarget(m_result); }

bool MSPauseBackdrop::init(CCNode* pause)
{
  if (!CCNode::init())
    return false;
  auto const& theme = Theme::get();
  m_pause = pause;
  this->setAnchorPoint({0, 0});
  this->setPosition({0, 0});

  // The tint over the whole picture, the gradient from the left over that
  m_tint = CCLayerColor::create(Theme::withAlpha(theme.pauseTintColor, theme.pauseTintOpacity));
  m_gradient = CCLayerGradient::create(Theme::withAlpha(theme.pauseGradientColor, theme.pauseGradientOpacity),
                                       Theme::withAlpha(theme.pauseGradientColor, 0), {1, 0});
  this->addChild(m_tint, 1);
  this->addChild(m_gradient, 2);

  m_frame = CCEGLView::sharedOpenGLView()->getFrameSize();
  m_pendingFrame = m_frame;
  if (!theme.pauseBlurEnabled || !this->blur())
    this->showDim();
  this->layout();
  this->scheduleUpdate();
  return true;
}

bool MSPauseBackdrop::isBlurred() const { return m_picture != nullptr; }

void MSPauseBackdrop::showDim()
{
  auto const& theme = Theme::get();
  if (m_picture)
  {
    m_picture->removeFromParent();
    m_picture = nullptr;
  }
  this->freeTarget(m_result);
  if (!m_dim)
  {
    m_dim = CCLayerColor::create(Theme::withAlpha(theme.pauseDimColor, theme.pauseDimOpacity));
    this->addChild(m_dim, 0);
  }
}

void MSPauseBackdrop::layout()
{
  auto winSize = CCDirector::get()->getWinSize();
  if (m_picture)
  {
    auto size = m_picture->getContentSize();
    if (size.width > 0 && size.height > 0)
    {
      m_picture->setScaleX(winSize.width / size.width);
      m_picture->setScaleY(winSize.height / size.height);
    }
  }
  if (m_dim)
    m_dim->setContentSize(winSize);
  m_tint->setContentSize(winSize);
  m_gradient->setContentSize({winSize.width * std::clamp(Theme::get().pauseGradientWidth, 0.f, 1.f), winSize.height});
}

bool MSPauseBackdrop::makeTarget(Target& target, int width, int height, bool depthStencil)
{
  target.width = width;
  target.height = height;
  auto texture = new CCTexture2D();
  if (!texture->initWithData(nullptr, kCCTexture2DPixelFormat_RGBA8888, width, height, CCSize(width, height)))
  {
    texture->release();
    return false;
  }
  // Clamped, so the blur reads the edge's own colour past it: no dark band at the borders
  ccTexParams params{GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE};
  texture->setTexParameters(&params);
  target.texture = texture;
  texture->release();

  GLint oldFbo = 0;
  GLint oldRbo = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFbo);
  glGetIntegerv(GL_RENDERBUFFER_BINDING, &oldRbo);
  glGenFramebuffers(1, &target.fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, target.fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->getName(), 0);
  if (depthStencil)
  {
    // The level may clip with the stencil
    glGenRenderbuffers(1, &target.depthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, target.depthStencil);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, target.depthStencil);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, target.depthStencil);
  }
  auto complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  glBindFramebuffer(GL_FRAMEBUFFER, oldFbo);
  glBindRenderbuffer(GL_RENDERBUFFER, oldRbo);
  if (!complete)
  {
    this->freeTarget(target);
    return false;
  }
  return true;
}

void MSPauseBackdrop::freeTarget(Target& target)
{
  if (target.fbo)
    glDeleteFramebuffers(1, &target.fbo);
  if (target.depthStencil)
    glDeleteRenderbuffers(1, &target.depthStencil);
  target.fbo = 0;
  target.depthStencil = 0;
  target.texture = nullptr;
  target.width = target.height = 0;
}

void MSPauseBackdrop::pass(CCGLProgram* program, CCTexture2D* input, Target const& output)
{
  static GLfloat const POSITIONS[] = {-1, -1, 1, -1, -1, 1, 1, 1};
  static GLfloat const TEXCOORDS[] = {0, 0, 1, 0, 0, 1, 1, 1};
  glBindFramebuffer(GL_FRAMEBUFFER, output.fbo);
  glViewport(0, 0, output.width, output.height);
  program->use();
  ccGLBindTexture2D(input->getName());
  // Every pixel replaced
  ccGLBlendFunc(GL_ONE, GL_ZERO);
  ccGLEnableVertexAttribs(kCCVertexAttribFlag_Position | kCCVertexAttribFlag_TexCoords);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glVertexAttribPointer(kCCVertexAttrib_Position, 2, GL_FLOAT, GL_FALSE, 0, POSITIONS);
  glVertexAttribPointer(kCCVertexAttrib_TexCoords, 2, GL_FLOAT, GL_FALSE, 0, TEXCOORDS);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  CC_INCREMENT_GL_DRAWS(1);
}

bool MSPauseBackdrop::blur()
{
  auto const& theme = Theme::get();
  auto shaders = programs();
  if (!shaders)
  {
    logOnce("its shaders don't compile here");
    return false;
  }
  auto level = PlayLayer::get();
  if (!level)
  {
    logOnce("no level to capture");
    return false;
  }
  auto director = CCDirector::get();
  auto winSize = director->getWinSize();
  // The window's real pixels, whatever the texture quality
  auto frame = CCEGLView::sharedOpenGLView()->getFrameSize();
  GLint maxSize = 0;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
  auto width = static_cast<int>(std::round(frame.width));
  auto height = static_cast<int>(std::round(frame.height));
  if (width < 16 || height < 16 || width > maxSize || height > maxSize || winSize.width <= 0)
  {
    logOnce(fmt::format("the window is {}x{} px", width, height));
    return false;
  }
  // 2, 4 or 8
  auto downsample = theme.pauseBlurDownsample >= 6 ? 8 : theme.pauseBlurDownsample >= 3 ? 4 : 2;
  auto began = std::chrono::steady_clock::now();

  // What cocos and the level expect back, all of it
  GLint oldFbo = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFbo);
  GLfloat oldClear[4];
  glGetFloatv(GL_COLOR_CLEAR_VALUE, oldClear);
  auto scissor = glIsEnabled(GL_SCISSOR_TEST);
  if (scissor)
    glDisable(GL_SCISSOR_TEST);
  auto restore = [&] {
    glBindFramebuffer(GL_FRAMEBUFFER, oldFbo);
    director->setViewport();
    glClearColor(oldClear[0], oldClear[1], oldClear[2], oldClear[3]);
    if (scissor)
      glEnable(GL_SCISSOR_TEST);
    // cocos' usual blending, through its cache
    ccGLBlendFunc(CC_BLEND_SRC, CC_BLEND_DST);
  };

  // The level at the window's full size, so whatever in it sets the viewport back to the window's still fits.
  // Freed as soon as it's downsampled
  Target full;
  if (!this->makeTarget(full, width, height, true))
  {
    restore();
    logOnce(fmt::format("can't make a {}x{} framebuffer", width, height));
    return false;
  }
  auto hidden = m_pause && m_pause->isVisible();
  if (hidden)
    m_pause->setVisible(false);
  glBindFramebuffer(GL_FRAMEBUFFER, full.fbo);
  kmGLMatrixMode(KM_GL_PROJECTION);
  kmGLPushMatrix();
  kmGLMatrixMode(KM_GL_MODELVIEW);
  kmGLPushMatrix();
  director->setProjection(director->getProjection());
  glViewport(0, 0, width, height);
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  level->visit();
  kmGLMatrixMode(KM_GL_PROJECTION);
  kmGLPopMatrix();
  kmGLMatrixMode(KM_GL_MODELVIEW);
  kmGLPopMatrix();
  if (hidden)
    m_pause->setVisible(true);
  auto peak = width * height * 8.0;

  // Halved until 1/downsample: each step averages 2×2
  Target current = full;
  // Only `current` holds it now, so freeing it frees the texture
  full.texture = nullptr;
  for (int factor = 1; factor < downsample; factor *= 2)
  {
    Target next;
    if (!this->makeTarget(next, std::max(1, current.width / 2), std::max(1, current.height / 2), false))
    {
      this->freeTarget(current);
      restore();
      logOnce("can't make a framebuffer to downsample into");
      return false;
    }
    if (factor == 1)
      peak += next.width * next.height * 4.0;
    this->pass(shaders->copy, current.texture, next);
    this->freeTarget(current);
    current = next;
  }

  // The Gaussian, across then down
  auto pixelsPerPoint = width / winSize.width;
  auto sigma = std::clamp(theme.pauseBlurRadius, 0.f, 60.f) * pixelsPerPoint / downsample;
  if (sigma > MAX_TAPS * 2 / 3.f)
  {
    logOnce(fmt::format("a blur of {:.1f} texels is more than it takes, {:.1f} instead", sigma, MAX_TAPS * 2 / 3.f));
    sigma = MAX_TAPS * 2 / 3.f;
  }
  auto taps = 1;
  if (sigma >= 0.3f)
  {
    Target across;
    if (!this->makeTarget(across, current.width, current.height, false))
    {
      this->freeTarget(current);
      restore();
      logOnce("can't make a framebuffer to blur into");
      return false;
    }
    float weights[MAX_TAPS + 1] = {};
    float offsets[MAX_TAPS + 1] = {};
    taps = gaussTaps(sigma, weights, offsets);
    auto gauss = shaders->gauss;
    gauss->use();
    // cocos 2.2 has no setter for float arrays: straight to GL, the program in use
    glUniform1fv(shaders->weights, MAX_TAPS + 1, weights);
    glUniform1fv(shaders->offsets, MAX_TAPS + 1, offsets);
    gauss->setUniformLocationWith1i(shaders->count, taps);
    gauss->setUniformLocationWith2f(shaders->step, 1.f / current.width, 0);
    this->pass(gauss, current.texture, across);
    gauss->use();
    gauss->setUniformLocationWith2f(shaders->step, 0, 1.f / current.height);
    this->pass(gauss, across.texture, current);
    this->freeTarget(across);
  }
  restore();
  auto submitted = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - began).count();
  // Once a pause: how long the GPU really took
  glFinish();
  auto finished = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - began).count();

  this->freeTarget(m_result);
  m_result = current;
  auto kept = m_result.width * m_result.height * 4.0;
  log::info("Pause blur: {}x{} px captured, blurred at {}x{} (sigma {:.1f} texels, {} taps a side), {:.2f} ms to "
            "submit, {:.2f} ms with the GPU done; render targets {:.2f} MiB at most, {:.2f} MiB kept",
            width, height, m_result.width, m_result.height, sigma, taps - 1, submitted, finished,
            mebibytes(peak + kept * 2), mebibytes(kept));

  if (!m_picture)
  {
    m_picture = CCSprite::createWithTexture(m_result.texture);
    m_picture->setAnchorPoint({0, 0});
    m_picture->setPosition({0, 0});
    // The framebuffer's rows go up
    m_picture->setFlipY(true);
    this->addChild(m_picture, 0);
  }
  else
  {
    m_picture->setTexture(m_result.texture);
    auto size = m_result.texture->getContentSize();
    m_picture->setTextureRect({0, 0, size.width, size.height});
  }
  if (m_dim)
  {
    m_dim->removeFromParent();
    m_dim = nullptr;
  }
  return true;
}

void MSPauseBackdrop::update(float dt)
{
  // A new window size, once it has settled: the picture made again (the level is still stopped)
  auto frame = CCEGLView::sharedOpenGLView()->getFrameSize();
  if (!frame.equals(m_pendingFrame))
  {
    m_pendingFrame = frame;
    m_settle = 0;
    return;
  }
  if (frame.equals(m_frame))
    return;
  m_settle += dt;
  if (m_settle < SETTLE_SECONDS)
    return;
  m_frame = frame;
  if (Theme::get().pauseBlurEnabled && !this->blur())
    this->showDim();
  this->layout();
}
