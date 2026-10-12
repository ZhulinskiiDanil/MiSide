#include "LevelPreview.hpp"

#include "../Theme.hpp"

#include <Geode/modify/PlayLayer.hpp>

#include <Geode/modify/GJBaseGameLayer.hpp>

#include <filesystem>
#include <map>
#include <thread>

using namespace geode::prelude;

namespace
{
  // The official levels' IDs
  constexpr int FIRST_LEVEL = 1;
  constexpr int LAST_LEVEL = 22;
  // The picture is taken once the attempt is past this share of the level and the player has clicked (there's
  // something going on), or at the latest at FORCE_PERCENT
  constexpr float CLICKED_PERCENT = 1;
  constexpr float FORCE_PERCENT = 5;

  std::filesystem::path previewPath(int levelID)
  {
    return Mod::get()->getSaveDir() / "previews" / fmt::format("level-{}.png", levelID);
  }

  bool official(GJGameLevel* level)
  {
    if (!level)
      return false;
    auto id = level->m_levelID.value();
    return level->m_levelType == GJLevelType::Main && id >= FIRST_LEVEL && id <= LAST_LEVEL;
  }

  std::map<int, Ref<CCTexture2D>> s_textures;

  // Shrunk by averaging to `outWidth` (the shape kept), rows turned to go down, saved as PNG. Pure CPU: off the
  // main thread
  bool shrinkAndSave(std::vector<uint8_t> const& pixels, int width, int height, int outWidth,
                     std::filesystem::path const& path)
  {
    auto outHeight = std::max(1, height * outWidth / width);
    std::vector<uint8_t> shrunk(static_cast<size_t>(outWidth) * outHeight * 4);
    for (int y = 0; y < outHeight; y++)
    {
      auto y0 = y * height / outHeight;
      auto y1 = std::max(y0 + 1, (y + 1) * height / outHeight);
      for (int x = 0; x < outWidth; x++)
      {
        auto x0 = x * width / outWidth;
        auto x1 = std::max(x0 + 1, (x + 1) * width / outWidth);
        unsigned sum[3] = {0, 0, 0};
        for (int sy = y0; sy < y1; sy++)
        {
          auto row = pixels.data() + (static_cast<size_t>(sy) * width + x0) * 4;
          for (int sx = x0; sx < x1; sx++, row += 4)
          {
            sum[0] += row[0];
            sum[1] += row[1];
            sum[2] += row[2];
          }
        }
        auto count = static_cast<unsigned>((y1 - y0) * (x1 - x0));
        auto out = shrunk.data() + (static_cast<size_t>(outHeight - 1 - y) * outWidth + x) * 4;
        out[0] = static_cast<uint8_t>(sum[0] / count);
        out[1] = static_cast<uint8_t>(sum[1] / count);
        out[2] = static_cast<uint8_t>(sum[2] / count);
        out[3] = 255;
      }
    }
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    auto image = new CCImage();
    auto saved = image->initWithImageData(shrunk.data(), static_cast<int>(shrunk.size()), CCImage::kFmtRawData,
                                          outWidth, outHeight, 8) &&
                 image->saveToFile(path.string().c_str(), false);
    image->release();
    return saved;
  }

  // The level drawn once at the window's size (so anything in it that sets the viewport back still fits) and read
  // back, its interface hidden meanwhile; the shrinking and saving go to a thread, so the game only pauses for the
  // drawing and reading
  bool capture(PlayLayer* level, int levelID)
  {
    auto director = CCDirector::get();
    auto frame = CCEGLView::sharedOpenGLView()->getFrameSize();
    auto width = static_cast<int>(frame.width);
    auto height = static_cast<int>(frame.height);
    GLint maxSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
    if (width < 16 || height < 16 || width > maxSize || height > maxSize)
      return false;

    std::vector<std::pair<CCNode*, bool>> hidden;
    for (CCNode* node : {static_cast<CCNode*>(level->m_uiLayer), static_cast<CCNode*>(level->m_attemptLabel),
                         static_cast<CCNode*>(level->m_percentageLabel), static_cast<CCNode*>(level->m_progressBar)})
    {
      if (node)
      {
        hidden.emplace_back(node, node->isVisible());
        node->setVisible(false);
      }
    }

    GLint oldFbo = 0;
    GLint oldRbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFbo);
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &oldRbo);
    GLfloat oldClear[4];
    glGetFloatv(GL_COLOR_CLEAR_VALUE, oldClear);
    auto scissor = glIsEnabled(GL_SCISSOR_TEST);
    if (scissor)
      glDisable(GL_SCISSOR_TEST);

    GLuint fbo = 0;
    GLuint color = 0;
    GLuint depth = 0;
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &color);
    glGenRenderbuffers(1, &depth);
    glBindRenderbuffer(GL_RENDERBUFFER, color);
#ifdef GL_RGBA8
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height);
#else
    // OpenGL ES 2 (Android): the same format by its extension's name
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, width, height);
#endif
    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth);
    std::vector<uint8_t> pixels;
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
    {
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
      pixels.resize(static_cast<size_t>(width) * height * 4);
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    }
    glBindFramebuffer(GL_FRAMEBUFFER, oldFbo);
    glBindRenderbuffer(GL_RENDERBUFFER, oldRbo);
    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &color);
    glDeleteRenderbuffers(1, &depth);
    director->setViewport();
    glClearColor(oldClear[0], oldClear[1], oldClear[2], oldClear[3]);
    if (scissor)
      glEnable(GL_SCISSOR_TEST);
    for (auto [node, visible] : hidden)
      node->setVisible(visible);
    if (pixels.empty())
      return false;

    auto outWidth = std::clamp(static_cast<int>(Theme::get().levelsPreviewWidth), 64, width);
    std::thread([pixels = std::move(pixels), width, height, outWidth, levelID] {
      auto saved = shrinkAndSave(pixels, width, height, outWidth, previewPath(levelID));
      Loader::get()->queueInMainThread([saved, levelID] {
        if (!saved)
          return log::warn("Level {}'s picture couldn't be saved", levelID);
        log::info("Level {}'s picture saved for the level list", levelID);
        s_textures.erase(levelID);
      });
    }).detach();
    return true;
  }
} // namespace

namespace preview
{
  CCTexture2D* texture(int levelID)
  {
    if (auto it = s_textures.find(levelID); it != s_textures.end())
      return it->second;
    std::error_code error;
    auto path = previewPath(levelID);
    CCTexture2D* result = nullptr;
    if (std::filesystem::exists(path, error))
    {
      auto image = new CCImage();
      if (image->initWithImageFile(path.string().c_str()))
      {
        auto texture = new CCTexture2D();
        if (texture->initWithImage(image))
        {
          texture->setAntiAliasTexParameters();
          result = texture;
        }
        texture->autorelease();
      }
      image->release();
    }
    s_textures[levelID] = result;
    return result;
  }

  void forget() { s_textures.clear(); }
} // namespace preview

// The first time an official level is played, a frame of it is kept for the level list: once the attempt is past 1%
// and the player has clicked, or at 5% at the latest
class $modify(MSPreviewPlayLayer, PlayLayer)
{
  struct Fields
  {
    bool m_wanted = false;
    bool m_clicked = false;
  };

  bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects)
  {
    if (!PlayLayer::init(level, useReplay, dontCreateObjects))
      return false;
    std::error_code error;
    m_fields->m_wanted = official(level) && !std::filesystem::exists(previewPath(level->m_levelID.value()), error);
    return true;
  }

  void resetLevel()
  {
    m_fields->m_clicked = false;
    PlayLayer::resetLevel();
  }

  void postUpdate(float dt)
  {
    PlayLayer::postUpdate(dt);
    auto& f = *m_fields.self();
    // From a start position, it isn't the level as it comes; a dead player's frame isn't one either
    if (!f.m_wanted || m_isTestMode || !m_level || !m_player1 || m_player1->m_isDead)
      return;
    auto percent = this->getCurrentPercent();
    if (percent < FORCE_PERCENT && (percent < CLICKED_PERCENT || !f.m_clicked))
      return;
    f.m_wanted = false;
    if (!capture(this, m_level->m_levelID.value()))
      log::warn("Level {}'s picture couldn't be made", m_level->m_levelID.value());
  }
};

// The player's first click of the attempt: from then on the picture can be taken
class $modify(MSPreviewGameLayer, GJBaseGameLayer)
{
  void handleButton(bool down, int button, bool isPlayer1)
  {
    GJBaseGameLayer::handleButton(down, button, isPlayer1);
    if (!down)
      return;
    if (auto level = PlayLayer::get(); level && static_cast<GJBaseGameLayer*>(level) == this)
      static_cast<MSPreviewPlayLayer*>(level)->m_fields->m_clicked = true;
  }
};
