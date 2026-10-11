#pragma once

#include <Geode/Geode.hpp>

#include <string>

// How the menu's pieces are made from the theme, with a fallback for every missing asset
namespace ms
{
  // A file the game can load: a sprite (with its -hd / -uhd variant), a font, a sound
  bool resourceExists(std::string const& name);

  // The theme's font, or the system font when it's missing (it has Cyrillic too)
  cocos2d::CCNode* createLabel(std::string const& text);

  // A 9-slice sprite tinted with the color, or a plain rectangle of that color when the sprite is missing
  cocos2d::CCNode* createPanel(std::string const& sprite, cocos2d::ccColor3B color, GLubyte opacity);

  // Silent when the sound is missing
  void playSound(std::string const& name);
} // namespace ms
