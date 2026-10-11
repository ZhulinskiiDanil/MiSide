#pragma once

#include <Geode/Geode.hpp>

#include <optional>
#include <string>
#include <vector>

// Everything the menu looks like: colors, fonts, sprites, sounds, spacing and timings. The defaults are in
// Theme.cpp; <config dir>/theme.json overrides any of them, and theme.defaults.json next to it lists them all.
// Positions are in points on the 569×320 screen, from the bottom left corner
struct Theme
{
  struct Layer
  {
    std::string sprite;
    // How far it follows the mouse: 0 stays, 1 moves by the whole parallax range
    float depth = 0;
    // Added to what's under it instead of covering it: for light drawn on black
    bool additive = false;
    GLubyte opacity = 255;
    // Multiplies the layer's colors. None: `background-tint` for the layers that cover, nothing for the added ones
    std::optional<cocos2d::ccColor3B> tint;
  };

  // ! --- Colors --- !
  cocos2d::ccColor3B backdropTop;
  cocos2d::ccColor3B backdropBottom;
  cocos2d::ccColor3B rowColor;
  cocos2d::ccColor3B rowHoverColor;
  cocos2d::ccColor3B rowPressedColor;
  cocos2d::ccColor3B rowDisabledColor;
  cocos2d::ccColor3B itemColor;
  cocos2d::ccColor3B itemHoverColor;
  cocos2d::ccColor3B itemPressedColor;
  cocos2d::ccColor3B subtitleColor;
  cocos2d::ccColor3B shadowColor;
  cocos2d::ccColor3B accentColor;
  cocos2d::ccColor3B backgroundTint;
  cocos2d::ccColor3B darkenColor;
  cocos2d::ccColor3B leftShadeColor;
  cocos2d::ccColor3B glowColor;
  cocos2d::ccColor3B fadeColor;
  cocos2d::ccColor3B trayColor;
  cocos2d::ccColor3B slotColor;
  cocos2d::ccColor3B cardColor;
  cocos2d::ccColor3B avatarColor;
  cocos2d::ccColor3B nameColor;
  cocos2d::ccColor3B popupColor;
  GLubyte rowOpacity = 0;
  GLubyte rowHoverOpacity = 0;
  GLubyte rowPressedOpacity = 0;
  GLubyte rowDisabledOpacity = 0;
  GLubyte itemOpacity = 0;
  GLubyte itemHoverOpacity = 0;
  // The nearest copy of the shadow; the farther ones are fainter
  GLubyte shadowOpacity = 0;
  // Over the background, under the menu
  GLubyte darkenOpacity = 0;
  // At the left edge, down to nothing at `left-shade-width`
  GLubyte leftShadeOpacity = 0;
  GLubyte glowOpacity = 0;
  GLubyte characterOpacity = 0;
  GLubyte fadeOpacity = 0;
  GLubyte trayOpacity = 0;
  GLubyte slotOpacity = 0;
  GLubyte cardOpacity = 0;
  GLubyte avatarOpacity = 0;
  GLubyte popupOpacity = 0;

  // ! --- Fonts --- !
  // A BMFont with Cyrillic; when it's missing, the system font below
  std::string font;
  // The subtitle's, heavier
  std::string titleFont;
  std::string fallbackFont;
  float fallbackFontSize = 0;
  float itemScale = 0;
  // Up from the row's middle: the font's capitals sit a little low in their line
  float itemTextOffset = 0;
  float subtitleScale = 0;
  float nameScale = 0;
  float moreScale = 0;
  float popupTitleScale = 0;
  // Copies of the text under it, each one `shadowOffset` further down and to the right
  float shadowSteps = 0;
  float shadowOffset = 0;

  // ! --- Sprites and sounds --- !
  // Back to front
  std::vector<Layer> layers;
  // A 9-slice plate under every item
  std::string rowSprite;
  // The logo image, shown as it is (its transparent margins left out). Missing: no logo
  std::string logoSprite;
  // White, soft round light tinted with `glow-color`
  std::string glowSprite;
  // A character in front of the room, on the right. Empty: none
  std::string characterSprite;
  std::string accentSprite;
  std::string traySprite;
  std::string slotSprite;
  std::string avatarSprite;
  std::string hoverSound;
  std::string clickSound;
  float soundVolume = 0;

  // ! --- Hover and the Exit button --- !
  // One light band through the letters when an item lights up: white in its middle, the tint at its edges,
  // only where the letters are. Its width is a share of the label's width; intensity 1 makes the middle white
  bool shineEnabled = false;
  cocos2d::ccColor3B shineTint;
  cocos2d::ccColor3B shineCoreColor;
  float shineDuration = 0;
  float shineBandWidth = 0;
  float shineIntensity = 0;
  // The white middle's share of the band
  float shineCore = 0;
  // How much the band leans: its top is this many label widths ahead of its bottom
  float shineSlant = 0;
  float shineFadeOut = 0;
  // A soft pink glow around the letters the band is on, under the label: about this many points out from them
  float shineGlowRadius = 0;
  float shineGlowIntensity = 0;
  cocos2d::ccColor3B shineGlowColor;
  // A soft glow around the highlighted row
  cocos2d::ccColor3B rowGlowColor;
  GLubyte rowGlowOpacity = 0;
  float rowGlowSpread = 0;
  // Exit leans away from the mouse, held to its place as if by a spring. Points on the reference screen
  bool exitDodgeEnabled = false;
  float exitDodgeRadius = 0;
  float exitDodgeMax = 0;
  // Its push up and down, as a share of the sideways one: it is wide and low, so it stays under the mouse
  float exitDodgeVertical = 0;
  // Stretched this much along the push at the full push, tilted up to this many degrees by its speed
  float exitDodgeStretch = 0;
  float exitDodgeTilt = 0;
  // The spring: how stiff, and how damped (1: no overshoot)
  float exitDodgeStiffness = 0;
  float exitDodgeDamping = 0;

  // ! --- The menu --- !
  // The left edge of the subtitle and the items
  float columnLeft = 0;
  // The logo's drawn part: its left edge (with the items), its bottom edge (over the subtitle) and its width
  float logoLeft = 0;
  float logoBottom = 0;
  float logoWidth = 0;
  // The subtitle's top edge
  float subtitleTop = 0;
  // The first item's middle, then one every `row-pitch` points down (from middle to middle)
  float firstRowY = 0;
  float rowPitch = 0;
  float rowWidth = 0;
  float rowHeight = 0;
  // The text's left edge in the row
  float textPadding = 0;
  // Quit stands apart, shorter, near the bottom
  float exitWidth = 0;
  float exitY = 0;
  // The least space between the menu and other mods' nodes
  float columnClearance = 0;
  bool accentEnabled = false;
  float accentWidth = 0;
  float accentGap = 0;
  float hoverShift = 0;
  float pressedScale = 0;

  // ! --- Decoration over the room --- !
  // A faint repeating pattern of hearts, stars and flowers (a white tile, tinted)
  std::string patternSprite;
  cocos2d::ccColor3B patternColor;
  GLubyte patternOpacity = 0;
  // One tile's size in points
  float patternTile = 0;
  // Dark edges
  std::string vignetteSprite;
  cocos2d::ccColor3B vignetteColor;
  GLubyte vignetteOpacity = 0;
  // A few small pink motes drifting slowly up
  float particleCount = 0;
  cocos2d::ccColor3B particleColor;
  GLubyte particleOpacity = 0;
  float particleSizeMin = 0;
  float particleSizeMax = 0;
  float particleSpeed = 0;
  float particleLife = 0;

  // ! --- The background --- !
  // The point of the art kept on screen when it's cropped to cover it, as shares of its width and height
  float backgroundFocusX = 0;
  float backgroundFocusY = 0;
  float leftShadeWidth = 0;
  // The glow's middle, as shares of the screen's width and height, and its size in points
  float glowX = 0;
  float glowY = 0;
  float glowWidth = 0;
  float glowHeight = 0;
  // The character's middle, as a share of the screen's width; its height, as a share of the screen's
  float characterX = 0;
  float characterHeight = 0;
  float characterDepth = 0;
  // How far the nearest layer moves at the full parallax strength
  float parallaxRange = 0;

  // ! --- Mita --- !
  // The animated character on the right, from resources/mita. Points on the reference screen (569×320)
  bool mitaEnabled = false;
  // Her bottom middle; held to the right edge on other window shapes, never over the menu
  float mitaX = 0;
  float mitaY = 0;
  // Points per pixel of her 640×720 canvas
  float mitaScale = 0;
  // 0: the manifest's
  float mitaFps = 0;
  bool mitaParallaxEnabled = false;
  float mitaParallaxMaxX = 0;
  // Down only, so the cut at the bottom of her frame never shows a gap
  float mitaParallaxMaxY = 0;
  // A frame to hold still on, to check one at a time; -1 plays
  float mitaFreezeFrame = 0;
  // A thin pink line around her outline, drawn by her own shader (only hers)
  bool mitaOutlineEnabled = false;
  cocos2d::ccColor3B mitaOutlineColor;
  GLubyte mitaOutlineOpacity = 0;
  // In her texture's pixels. Her frames have wide transparent sides; their top and bottom edges (where the
  // atlas has only 2 pixels to the next frame) are off the screen
  float mitaOutlineWidth = 0;

  // ! --- The tray and the profile --- !
  float trayMargin = 0;
  float trayPadding = 0;
  float trayGap = 0;
  // Every button in the tray is scaled to fit this square
  float slotSize = 0;
  float slotPadding = 0;
  // The share of the screen's width the tray's row may take; more buttons make every slot smaller
  float trayMaxWidth = 0;
  // The profile card's bottom left corner
  float cardLeft = 0;
  float cardBottom = 0;
  float cardPadding = 0;
  float cardGap = 0;
  float avatarSize = 0;
  // The player's icon in the avatar, as a share of the avatar's size
  float avatarIcon = 0;
  // The card grows this much while pressed
  float cardPressScale = 0;
  float popupSlotSize = 0;
  float popupColumns = 0;
  float popupGap = 0;
  float popupPadding = 0;

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
