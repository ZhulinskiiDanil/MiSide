#pragma once

#include <Geode/Geode.hpp>

#include <optional>
#include <string>

// How the menu's pieces are made from the theme, with a fallback for every missing asset
namespace ms
{
  // A file the game can load: a sprite (with its -hd / -uhd variant), a font, a sound
  bool resourceExists(std::string const& name);

  // The theme's font (or `font`), or the system font when it's missing (it has Cyrillic too)
  cocos2d::CCNode* createLabel(std::string const& text, std::string const& font = "");

  struct Text
  {
    // The label with its shadow under it. Its opacity fades both; anchored at the left middle
    cocos2d::CCNodeRGBA* node = nullptr;
    // The label itself, to tint
    cocos2d::CCNode* label = nullptr;
  };

  // A text's shadow: `steps` copies down and to the right, each `offset` further and fainter; 0 steps: none
  struct Shadow
  {
    cocos2d::ccColor3B color;
    GLubyte opacity = 0;
    int steps = 0;
    float offset = 0;

    // The theme's soft one
    static Shadow theme();
  };

  // The theme's font with the theme's soft shadow: a few fainter copies of the text, down and to the right
  Text createText(std::string const& text, float scale, cocos2d::ccColor3B color, std::string const& font = "");
  Text createText(std::string const& text, float scale, cocos2d::ccColor3B color, std::string const& font,
                  Shadow const& shadow);

  // Why the node can't be used now: it isn't in the running scene, or a popup (an FLAlertLayer) is over it.
  // Nothing in the way: nullopt. During a scene transition: an empty reason (not worth a log)
  std::optional<std::string> coveredBy(cocos2d::CCNode* node);

  // Opacity set on the node reaches its children, and theirs
  void cascadeOpacity(cocos2d::CCNode* node);
  void setOpacity(cocos2d::CCNode* node, GLubyte opacity);
  void setColor(cocos2d::CCNode* node, cocos2d::ccColor3B color);

  // A 9-slice sprite tinted with the color, or a plain rectangle of that color when the sprite is missing
  cocos2d::CCNode* createPanel(std::string const& sprite, cocos2d::ccColor3B color, GLubyte opacity);

  // A sprite stretched to the size and tinted, or a plain rectangle when the sprite is missing. For pieces
  // thinner than a 9-slice's corners
  cocos2d::CCNode* createStretched(std::string const& sprite, cocos2d::CCSize size, cocos2d::ccColor3B color,
                                   GLubyte opacity);

  // A sprite shader program of the menu's own (never put in the shader cache), with the usual attributes bound and
  // linked; autoreleased. Null when it doesn't compile or link, logged with the driver's messages and `what`
  cocos2d::CCGLProgram* createProgram(char const* vertex, char const* fragment, char const* what);

  // Silent when the sound is missing
  void playSound(std::string const& name);
} // namespace ms
