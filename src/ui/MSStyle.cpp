#include "MSStyle.hpp"

#include "../Theme.hpp"

using namespace geode::prelude;

namespace ms
{
  bool resourceExists(std::string const& name)
  {
    if (name.empty())
      return false;
    auto files = CCFileUtils::get();
    return files->isFileExist(files->fullPathForFilename(name.c_str(), false));
  }

  CCNode* createLabel(std::string const& text)
  {
    auto const& theme = Theme::get();
    if (resourceExists(theme.font))
    {
      if (auto label = CCLabelBMFont::create(text.c_str(), theme.font.c_str()))
        return label;
    }
    static bool s_warned = false;
    if (!s_warned)
    {
      s_warned = true;
      log::warn("Font {} not found, the menu uses the system font {}", theme.font, theme.fallbackFont);
    }
    return CCLabelTTF::create(text.c_str(), theme.fallbackFont.c_str(), theme.fallbackFontSize);
  }

  CCNode* createPanel(std::string const& sprite, ccColor3B color, GLubyte opacity)
  {
    if (resourceExists(sprite))
    {
      if (auto panel = CCScale9Sprite::create(sprite.c_str()))
      {
        panel->setColor(color);
        panel->setOpacity(opacity);
        return panel;
      }
    }
    log::warn("Sprite {} not found, a plain rectangle instead", sprite);
    return CCLayerColor::create(Theme::withAlpha(color, opacity));
  }

  void playSound(std::string const& name)
  {
    if (!resourceExists(name))
      return;
    FMODAudioEngine::get()->playEffect(name, 1.f, 0.f, Theme::get().soundVolume);
  }
} // namespace ms
