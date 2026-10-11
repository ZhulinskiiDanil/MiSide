#pragma once

#include <Geode/Geode.hpp>

#include <string>
#include <vector>

// Everything the menu looks like: colors, fonts, sprites, sounds, spacing and timings. The defaults are in
// Theme.cpp; <config dir>/theme.json overrides any of them, and theme.defaults.json next to it lists them all
struct Theme
{
  struct Layer
  {
    std::string sprite;
    // How far it follows the mouse: 0 stays, 1 moves by the whole parallax range
    float depth = 0;
    // Added to what's under it instead of covering it: for light drawn on black
    bool additive = false;
  };

  // ! --- Colors --- !
  cocos2d::ccColor3B backdropTop;
  cocos2d::ccColor3B backdropBottom;
  cocos2d::ccColor3B itemColor;
  cocos2d::ccColor3B itemHoverColor;
  cocos2d::ccColor3B itemPressedColor;
  cocos2d::ccColor3B accentColor;
  cocos2d::ccColor3B fadeColor;
  cocos2d::ccColor3B trayColor;
  cocos2d::ccColor3B slotColor;
  GLubyte itemOpacity = 0;
  GLubyte itemHoverOpacity = 0;
  GLubyte fadeOpacity = 0;
  GLubyte trayOpacity = 0;
  GLubyte slotOpacity = 0;

  // ! --- Fonts --- !
  // A BMFont with Cyrillic; when it's missing, the system font below
  std::string font;
  std::string fallbackFont;
  float fallbackFontSize = 0;
  float itemScale = 0;

  // ! --- Sprites and sounds --- !
  // Back to front
  std::vector<Layer> layers;
  std::string accentSprite;
  std::string traySprite;
  std::string slotSprite;
  std::string hoverSound;
  std::string clickSound;
  float soundVolume = 0;

  // ! --- Spacing --- !
  float columnLeft = 0;
  // The column's top edge at most, as a share of the screen's height from the bottom
  float columnTop = 0;
  // The least space between the column and anything else on the screen
  float columnClearance = 0;
  float itemGap = 0;
  float accentWidth = 0;
  float accentGap = 0;
  float hoverShift = 0;
  float pressedScale = 0;
  float trayMargin = 0;
  float trayPadding = 0;
  float trayGap = 0;
  // Every button in the tray is scaled to fit this square
  float slotSize = 0;
  float slotPadding = 0;
  // Slots in a row of the tray; more buttons start a new row above
  float trayColumns = 0;
  // The share of the screen's height the tray may take; more rows make every slot smaller
  float trayMaxHeight = 0;
  // How far the nearest layer moves at the full parallax strength
  float parallaxRange = 0;

  // ! --- Timings --- !
  float hoverDuration = 0;
  float introDuration = 0;
  float introStagger = 0;
  float introShift = 0;
  float fadeDuration = 0;
  float fadeBackDuration = 0;
  // The background reaches the mouse's offset in about this many seconds
  float parallaxSmoothing = 0;

  static Theme const& get();
  // Defaults, then theme.json. Called at every main menu, so an edited theme.json shows without a restart
  static void reload();
  static cocos2d::ccColor4B withAlpha(cocos2d::ccColor3B color, GLubyte opacity);
};
