#include "MSShine.hpp"

#include "../Theme.hpp"
#include "MSStyle.hpp"

#include <array>
#include <cmath>
#include <vector>

using namespace geode::prelude;

namespace
{
  // The usual sprite vertex shader
  constexpr char const* SHINE_VERTEX = R"(
attribute vec4 a_position;
attribute vec2 a_texCoord;
attribute vec4 a_color;
#ifdef GL_ES
varying lowp vec4 v_fragmentColor;
varying mediump vec2 v_texCoord;
#else
varying vec4 v_fragmentColor;
varying vec2 v_texCoord;
#endif
void main()
{
  gl_Position = CC_MVPMatrix * a_position;
  v_fragmentColor = a_color;
  v_texCoord = a_texCoord;
}
)";

  // Both layers place the band the same way: the texture coordinate, divided by where the texture's content ends,
  // turned into label widths (the label sits inside the texture with room around it). cocos 2.2 puts only the
  // matrices before a shader, so the texture is declared here. Their textures are white and premultiplied: only
  // alpha matters. Out: premultiplied
  constexpr char const* BAND = R"(
#ifdef GL_ES
precision mediump float;
#endif
varying vec4 v_fragmentColor;
varying vec2 v_texCoord;
uniform sampler2D CC_Texture0;
uniform vec2 u_extent;
uniform vec4 u_frame;
uniform float u_center;
uniform float u_width;
uniform float u_strength;
uniform float u_slant;
uniform float u_plateau;
// The wave's light: full on its flat middle (u_plateau of its half width), then a long smooth fall to nothing at
// its edge. No sharp core, no visible border
float wave()
{
  vec2 along = (v_texCoord / u_extent) * u_frame.zw - u_frame.xy;
  float distance = abs(along.x + (along.y - 0.5) * u_slant - u_center);
  float halfWidth = max(u_width * 0.5, 0.0001);
  return 1.0 - smoothstep(halfWidth * clamp(u_plateau, 0.0, 0.95), halfWidth, distance);
}
)";

  // The letters: their own alpha, near white in the wave's middle, through the soft tint back to their own color
  // at its edges (where it's nearly transparent). Drawn over the glow, sharp
  constexpr char const* CORE_FRAGMENT = R"(
uniform vec3 u_tint;
uniform vec3 u_coreColor;
void main()
{
  float letters = texture2D(CC_Texture0, v_texCoord).a;
  float light = wave();
  float alpha = clamp(letters * light * u_strength, 0.0, 1.0);
  gl_FragColor = vec4(mix(u_tint, u_coreColor, light) * alpha, alpha) * v_fragmentColor;
}
)";

  // The glow: the blurred letters (already reaching past them) times the band, in the glow color. Not multiplied
  // by the letters' own alpha, or it would be cut back to them
  constexpr char const* GLOW_FRAGMENT = R"(
uniform vec3 u_glowColor;
void main()
{
  float halo = texture2D(CC_Texture0, v_texCoord).a;
  float alpha = clamp(halo * wave() * u_strength, 0.0, 1.0);
  gl_FragColor = vec4(u_glowColor * alpha, alpha) * v_fragmentColor;
}
)";

  struct Shader
  {
    Ref<CCGLProgram> program;
    GLint extent = -1;
    GLint frame = -1;
    GLint center = -1;
    GLint width = -1;
    GLint strength = -1;
    GLint slant = -1;
    GLint plateau = -1;
    GLint tint = -1;
    GLint coreColor = -1;
    GLint glowColor = -1;
  };

  Shader make(char const* fragment, char const* what)
  {
    Shader shader;
    auto source = std::string(BAND) + fragment;
    auto program = ms::createProgram(SHINE_VERTEX, source.c_str(), what);
    if (!program)
      return shader;
    shader.program = program;
    shader.extent = program->getUniformLocationForName("u_extent");
    shader.frame = program->getUniformLocationForName("u_frame");
    shader.center = program->getUniformLocationForName("u_center");
    shader.width = program->getUniformLocationForName("u_width");
    shader.strength = program->getUniformLocationForName("u_strength");
    shader.slant = program->getUniformLocationForName("u_slant");
    shader.plateau = program->getUniformLocationForName("u_plateau");
    shader.tint = program->getUniformLocationForName("u_tint");
    shader.coreColor = program->getUniformLocationForName("u_coreColor");
    shader.glowColor = program->getUniformLocationForName("u_glowColor");
    return shader;
  }

  // Made once for the session, on the main thread; null when either doesn't compile
  Shader const* shader(MSShine::Layer layer)
  {
    static Shader s_core;
    static Shader s_glow;
    static bool s_tried = false;
    if (!s_tried)
    {
      s_tried = true;
      s_core = make(CORE_FRAGMENT, "The shine's core shader (the items have no shine)");
      s_glow = make(GLOW_FRAGMENT, "The shine's glow shader (the items have no shine)");
    }
    if (!s_core.program || !s_glow.program)
      return nullptr;
    return layer == MSShine::Layer::Core ? &s_core : &s_glow;
  }

  // One box blur pass along rows or columns, out of bounds counting as transparent
  void boxPass(std::vector<float> const& in, std::vector<float>& out, int width, int height, int radius, bool rows)
  {
    auto lines = rows ? height : width;
    auto length = rows ? width : height;
    auto stride = rows ? 1 : width;
    auto window = static_cast<float>(radius * 2 + 1);
    for (int line = 0; line < lines; line++)
    {
      auto base = rows ? line * width : line;
      float sum = 0;
      for (int i = 0; i <= radius && i < length; i++)
        sum += in[base + i * stride];
      for (int i = 0; i < length; i++)
      {
        out[base + i * stride] = sum / window;
        auto leaving = i - radius;
        auto coming = i + radius + 1;
        if (leaving >= 0)
          sum -= in[base + leaving * stride];
        if (coming < length)
          sum += in[base + coming * stride];
      }
    }
  }

  // Three box passes each way: close to a Gaussian with sigma about radius + 0.5. One scratch buffer, made once
  std::vector<float> blur(std::vector<float> alpha, int width, int height, int radius)
  {
    if (radius <= 0)
      return alpha;
    std::vector<float> scratch(alpha.size());
    for (int pass = 0; pass < 3; pass++)
    {
      boxPass(alpha, scratch, width, height, radius, true);
      boxPass(scratch, alpha, width, height, radius, false);
    }
    return alpha;
  }

  // A white, premultiplied texture from an alpha map (rows from the top, as cocos textures are)
  CCTexture2D* whiteTexture(std::vector<float> const& alpha, int width, int height)
  {
    std::vector<uint8_t> pixels(alpha.size() * 4);
    for (size_t i = 0; i < alpha.size(); i++)
    {
      auto value = static_cast<uint8_t>(std::clamp(alpha[i], 0.f, 1.f) * 255.f + 0.5f);
      pixels[i * 4 + 0] = pixels[i * 4 + 1] = pixels[i * 4 + 2] = pixels[i * 4 + 3] = value;
    }
    auto texture = new CCTexture2D();
    if (!texture->initWithData(pixels.data(), kCCTexture2DPixelFormat_RGBA8888, width, height, CCSize(width, height)))
    {
      texture->release();
      return nullptr;
    }
    texture->setAntiAliasTexParameters();
    texture->autorelease();
    return texture;
  }
} // namespace

MSShine::Pair MSShine::create(std::string const& text, float scale, std::string const& font)
{
  if (!shader(Layer::Core))
    return {};
  auto const& theme = Theme::get();
  auto label = ms::createLabel(text, font);
  if (!label)
    return {};
  label->setScale(scale);
  label->setAnchorPoint({0, 0});
  ms::setColor(label, ccWHITE);
  auto size = label->getScaledContentSize();
  if (size.width <= 0 || size.height <= 0)
    return {};

  // Room around the label for the glow, in whole points; rendered once at the label's size (the content scale
  // factor's pixels: as sharp as the label itself)
  auto pad = std::ceil(std::max(theme.shineGlowRadius, 0.f) * 1.5f) + 1;
  auto width = static_cast<int>(std::ceil(size.width + pad * 2));
  auto height = static_cast<int>(std::ceil(size.height + pad * 2));
  auto render = CCRenderTexture::create(width, height);
  if (!render)
    return {};
  label->setPosition({pad, pad});
  render->beginWithClear(0, 0, 0, 0);
  label->visit();
  render->end();
  // Read back once: rows from the top
  auto image = render->newCCImage(true);
  if (!image || !image->getData())
  {
    CC_SAFE_RELEASE(image);
    return {};
  }
  int pixelsWide = image->getWidth();
  int pixelsHigh = image->getHeight();
  std::vector<float> alpha(static_cast<size_t>(pixelsWide) * pixelsHigh);
  auto data = image->getData();
  for (size_t i = 0; i < alpha.size(); i++)
    alpha[i] = data[i * 4 + 3] / 255.f;
  image->release();

  // The glow's blur in this texture quality's pixels: about `shine-glow-radius` points on the screen
  auto pixelsPerPoint = pixelsWide / static_cast<float>(width);
  auto radius = static_cast<int>(std::round(std::max(theme.shineGlowRadius, 0.f) * pixelsPerPoint / 2 - 0.5f));
  auto sharp = whiteTexture(alpha, pixelsWide, pixelsHigh);
  auto soft = whiteTexture(blur(alpha, pixelsWide, pixelsHigh, std::max(radius, 0)), pixelsWide, pixelsHigh);
  if (!sharp || !soft)
    return {};

  // Where the label is in the textures, in label widths and heights
  CCPoint offset{pad / size.width, pad / size.height};
  auto labelWidth = size.width;
  CCSize span{width / size.width, height / size.height};
  Pair pair;
  for (auto layer : {Layer::Core, Layer::Glow})
  {
    auto sprite = new MSShine();
    if (!sprite->initWithTexture(layer == Layer::Core ? sharp : soft))
    {
      delete sprite;
      return {};
    }
    sprite->autorelease();
    sprite->m_layer = layer;
    sprite->m_offset = offset;
    sprite->m_span = span;
    sprite->m_labelWidth = labelWidth;
    // Premultiplied: the core covers like any text; the glow adds light
    sprite->setBlendFunc(layer == Layer::Core ? ccBlendFunc{GL_ONE, GL_ONE_MINUS_SRC_ALPHA}
                                              : ccBlendFunc{GL_ONE, GL_ONE});
    sprite->setOpacityModifyRGB(true);
    sprite->setShaderProgram(shader(layer)->program);
    sprite->setAnchorPoint({0, 0});
    // The label at the parent's origin, the room around it
    sprite->setPosition({-pad, -pad});
    sprite->setVisible(false);
    (layer == Layer::Core ? pair.core : pair.glow) = sprite;
  }
  return pair;
}

float MSShine::margin() const
{
  return m_labelWidth > 0 ? std::max(Theme::get().shineGlowRadius, 0.f) / m_labelWidth : 0.f;
}

void MSShine::setPass(float center, float strength)
{
  m_center = center;
  m_strength = strength;
  this->setVisible(strength > 0);
}

void MSShine::draw()
{
  auto shine = shader(m_layer);
  if (!shine || m_strength <= 0)
    return;
  auto const& theme = Theme::get();
  auto program = shine->program.data();
  // This layer's pass, right before it draws: every item has its own
  program->use();
  auto texture = this->getTexture();
  program->setUniformLocationWith2f(shine->extent, std::max(texture->getMaxS(), 0.0001f),
                                    std::max(texture->getMaxT(), 0.0001f));
  program->setUniformLocationWith4f(shine->frame, m_offset.x, m_offset.y, m_span.width, m_span.height);
  program->setUniformLocationWith1f(shine->center, m_center);
  program->setUniformLocationWith1f(shine->width, theme.shineBandWidth);
  program->setUniformLocationWith1f(shine->slant, theme.shineSlant);
  program->setUniformLocationWith1f(shine->plateau, std::clamp(theme.shineCore, 0.f, 0.95f));
  auto rgb = [](ccColor3B color) { return std::array<float, 3>{color.r / 255.f, color.g / 255.f, color.b / 255.f}; };
  if (m_layer == Layer::Core)
  {
    program->setUniformLocationWith1f(shine->strength, m_strength);
    auto tint = rgb(theme.shineTint);
    auto core = rgb(theme.shineCoreColor);
    program->setUniformLocationWith3f(shine->tint, tint[0], tint[1], tint[2]);
    program->setUniformLocationWith3f(shine->coreColor, core[0], core[1], core[2]);
  }
  else
  {
    // The pass' strength (it fades when the mouse leaves) times the glow's own
    program->setUniformLocationWith1f(shine->strength, m_strength * theme.shineGlowIntensity);
    auto glow = rgb(theme.shineGlowColor);
    program->setUniformLocationWith3f(shine->glowColor, glow[0], glow[1], glow[2]);
  }
  CCSprite::draw();
}
