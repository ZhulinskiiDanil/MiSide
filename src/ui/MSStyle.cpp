#include "MSStyle.hpp"

#include "../Theme.hpp"

#include <typeinfo>

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

  Shadow Shadow::theme()
  {
    auto const& theme = Theme::get();
    return {theme.shadowColor, theme.shadowOpacity, static_cast<int>(theme.shadowSteps), theme.shadowOffset};
  }

  Text createText(std::string const& text, float scale, ccColor3B color, std::string const& font)
  {
    return createText(text, scale, color, font, Shadow::theme());
  }

  Text createText(std::string const& text, float scale, ccColor3B color, std::string const& font,
                  Shadow const& look)
  {
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
    auto steps = std::max(look.steps, 0);
    for (auto step = steps; step >= 1; step--)
    {
      auto shadow = createLabel(text, font);
      if (!shadow)
        continue;
      shadow->setScale(scale);
      shadow->setAnchorPoint({0, 0});
      shadow->setPosition({look.offset * step, -look.offset * step});
      cascadeOpacity(shadow);
      setColor(shadow, look.color);
      setOpacity(shadow, static_cast<GLubyte>(look.opacity / step));
      node->addChild(shadow);
    }
    node->addChild(label);
    return {node, label};
  }

  std::optional<std::string> coveredBy(CCNode* node)
  {
    // The scene the node is in, and the scene's child that holds it (a mod may wrap it in a node)
    CCNode* branch = node ? node->getParent() : nullptr;
    if (!branch)
      return "it isn't in a scene";
    while (branch->getParent() && !typeinfo_cast<CCScene*>(branch->getParent()))
      branch = branch->getParent();
    auto scene = typeinfo_cast<CCScene*>(branch->getParent());
    auto director = CCDirector::get();
    auto running = director->getRunningScene();
    // A scene transition: nothing to use
    if (director->getNextScene() || typeinfo_cast<CCTransitionScene*>(running))
      return "";
    if (!scene || running != scene)
      return "it isn't in the running scene";
    // A popup over it: GD's and Geode's popups are all FLAlertLayers. Other layers mods keep in the scene
    // (overlays, effects) don't count
    auto above = false;
    for (auto child : CCArrayExt<CCNode*>(scene->getChildren()))
    {
      if (child == branch)
      {
        above = true;
        continue;
      }
      if (above && child->isVisible() && typeinfo_cast<FLAlertLayer*>(child))
        return fmt::format("a popup ({}) is open", typeid(*child).name());
    }
    return std::nullopt;
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

  CCGLProgram* createProgram(char const* vertex, char const* fragment, char const* what)
  {
    auto program = new CCGLProgram();
    // It reports a shader that didn't compile only by failing to link, and keeps the messages
    program->initWithVertexShaderByteArray(vertex, fragment);
    program->addAttribute(kCCAttributeNamePosition, kCCVertexAttrib_Position);
    program->addAttribute(kCCAttributeNameColor, kCCVertexAttrib_Color);
    program->addAttribute(kCCAttributeNameTexCoord, kCCVertexAttrib_TexCoords);
    if (!program->link())
    {
      auto text = [](char const* log) { return std::string(log ? log : ""); };
      log::error("{} doesn't compile here. Vertex shader: {} Fragment shader: {} Program: {}", what,
                 text(program->vertexShaderLog()), text(program->fragmentShaderLog()), text(program->programLog()));
      program->release();
      return nullptr;
    }
    program->updateUniforms();
    program->autorelease();
    return program;
  }

  void playSound(std::string const& name)
  {
    if (!resourceExists(name))
      return;
    FMODAudioEngine::get()->playEffect(name, 1.f, 0.f, Theme::get().soundVolume);
  }
} // namespace ms
