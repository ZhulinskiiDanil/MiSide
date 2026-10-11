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

  CCNode* createLabel(std::string const& text, std::string const& font)
  {
    auto const& theme = Theme::get();
    auto const& file = font.empty() ? theme.font : font;
    if (resourceExists(file))
    {
      if (auto label = CCLabelBMFont::create(text.c_str(), file.c_str()))
        return label;
    }
    static bool s_warned = false;
    if (!s_warned)
    {
      s_warned = true;
      log::warn("Font {} not found, the menu uses the system font {}", file, theme.fallbackFont);
    }
    return CCLabelTTF::create(text.c_str(), theme.fallbackFont.c_str(), theme.fallbackFontSize);
  }

  bool canClip()
  {
    // Asked once: the window's buffers don't change
    static int s_bits = -1;
    if (s_bits < 0)
    {
      GLint bits = 0;
      glGetIntegerv(GL_STENCIL_BITS, &bits);
      s_bits = bits;
      if (s_bits <= 0)
        log::warn("No stencil buffer: the shine on the items is left out");
    }
    return s_bits > 0;
  }

  void cascadeOpacity(CCNode* node)
  {
    if (auto rgba = typeinfo_cast<CCRGBAProtocol*>(node))
      rgba->setCascadeOpacityEnabled(true);
  }

  void setOpacity(CCNode* node, GLubyte opacity)
  {
    if (auto rgba = typeinfo_cast<CCRGBAProtocol*>(node))
      rgba->setOpacity(opacity);
  }

  void setColor(CCNode* node, ccColor3B color)
  {
    if (auto rgba = typeinfo_cast<CCRGBAProtocol*>(node))
      rgba->setColor(color);
  }

  Text createText(std::string const& text, float scale, ccColor3B color, std::string const& font)
  {
    auto const& theme = Theme::get();
    auto label = createLabel(text, font);
    if (!label)
      return {};
    label->setScale(scale);
    label->setAnchorPoint({0, 0});
    cascadeOpacity(label);
    setColor(label, color);
    auto size = label->getScaledContentSize();

    auto node = CCNodeRGBA::create();
    node->setCascadeOpacityEnabled(true);
    node->setContentSize(size);
    node->setAnchorPoint({0, 0.5f});
    // Farthest first, so the nearer copies are drawn over it
    auto steps = std::max(static_cast<int>(theme.shadowSteps), 0);
    for (auto step = steps; step >= 1; step--)
    {
      auto shadow = createLabel(text, font);
      if (!shadow)
        continue;
      shadow->setScale(scale);
      shadow->setAnchorPoint({0, 0});
      shadow->setPosition({theme.shadowOffset * step, -theme.shadowOffset * step});
      cascadeOpacity(shadow);
      setColor(shadow, theme.shadowColor);
      setOpacity(shadow, static_cast<GLubyte>(theme.shadowOpacity / step));
      node->addChild(shadow);
    }
    node->addChild(label);
    return {node, label};
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

  CCNode* createStretched(std::string const& sprite, CCSize size, ccColor3B color, GLubyte opacity)
  {
    if (resourceExists(sprite))
    {
      if (auto node = CCSprite::create(sprite.c_str()))
      {
        auto own = node->getContentSize();
        if (own.width > 0 && own.height > 0)
        {
          node->setScaleX(size.width / own.width);
          node->setScaleY(size.height / own.height);
        }
        node->setColor(color);
        node->setOpacity(opacity);
        return node;
      }
    }
    log::warn("Sprite {} not found, a plain rectangle instead", sprite);
    auto rectangle = CCLayerColor::create(Theme::withAlpha(color, opacity));
    rectangle->setContentSize(size);
    rectangle->ignoreAnchorPointForPosition(false);
    return rectangle;
  }

  void playSound(std::string const& name)
  {
    if (!resourceExists(name))
      return;
    FMODAudioEngine::get()->playEffect(name, 1.f, 0.f, Theme::get().soundVolume);
  }
} // namespace ms
