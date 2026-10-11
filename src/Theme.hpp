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
  // One wide soft wave of light through the letters when an item lights up: near white in its middle, through the
  // tint back to the letters' color at its long soft edges. Its width is a share of the label's width
  bool shineEnabled = false;
  cocos2d::ccColor3B shineTint;
  cocos2d::ccColor3B shineCoreColor;
  float shineDuration = 0;
  float shineBandWidth = 0;
  float shineIntensity = 0;
  // The wave's flat bright middle, as a share of its half width; the rest is a long smooth fall
  float shineCore = 0;
  // How much the band leans: its top is this many label widths ahead of its bottom
  float shineSlant = 0;
  float shineFadeOut = 0;
  // 0..1: holds the wave at that point of the pass while an item is lit, to compare; -1: it moves
  float shineFreeze = 0;
  // A bloom around the letters the wave is on, under the label: a wide soft halo about this many points out from
  // them, and a tight bright one hugging them. The intensities bring each back up after the blur spreads the letters
  // thin; past about 1 it's white by them
  float shineGlowRadius = 0;
  float shineGlowIntensity = 0;
  float shineGlowInnerRadius = 0;
  float shineGlowInnerIntensity = 0;
  // How much wider the glow's wave is than the letters', so the halo rounds off at its sides
  float shineGlowSpread = 0;
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

  // ! --- The quit confirmation --- !
  // The column's own state, in MenuLayer and PauseLayer alike: a title in place of the subtitle and the list, and
  // two wide buttons under it. As in MiSide's: points of the 569×320 screen
  // The title: its left edge and the top of its capitals; white with a solid dark shadow
  float confirmTitleLeft = 0;
  float confirmTitleCapTop = 0;
  float confirmTitleScale = 0;
  cocos2d::ccColor3B confirmTitleColor;
  cocos2d::ccColor3B confirmTitleShadowColor;
  GLubyte confirmTitleShadowOpacity = 0;
  float confirmTitleShadowOffset = 0;
  // The buttons: the first one's top edge, then one every `confirm-row-pitch` points down
  float confirmRowLeft = 0;
  float confirmFirstRowTop = 0;
  float confirmRowPitch = 0;
  float confirmRowWidth = 0;
  float confirmRowHeight = 0;
  // Their plates at rest (lit and pressed, they're the items' own), and their text: lighter pink, no shadow
  cocos2d::ccColor3B confirmRowColor;
  GLubyte confirmRowOpacity = 0;
  cocos2d::ccColor3B confirmTextColor;
  // MiSide's buttons here are as heavy as the title: the title's font
  std::string confirmTextFont;
  float confirmTextScale = 0;
  float confirmTextOffset = 0;
  float confirmTextPadding = 0;
  // The list fading out and the confirmation in, and back; nothing is chosen before it has faded in
  float confirmDuration = 0;

  // ! --- Sounds --- !
  // MiSide's, played as GD's effects (the SFX volume counts, times `sound-volume`). Empty: none. Achievement and
  // win take the place of GD's own achievement_01 and endStart_02, so they never sound twice
  std::string toggleSound;
  std::string tabSound;
  std::string moreOpenSound;
  std::string moreCloseSound;
  std::string outfitSound;
  std::string achievementSound;
  std::string winSound;
  // Once when the pause opens, on a channel of its own, stopped when it closes
  std::string pauseStartSound;
  float pauseStartVolume = 0;

  // ! --- Music --- !
  // Played by GD itself, on its music channel: the main menu's in place of GD's menu loop (so it goes on through the
  // menus, stops for levels and follows the music volume and mute), the icon kit's while it's open. A file name in
  // the mod's resources or config folder, or a path. Volume: relative to the music volume
  bool menuMusicEnabled = false;
  std::string menuMusicPath;
  float menuMusicVolume = 0;
  bool garageMusicEnabled = false;
  std::string garageMusicPath;
  float garageMusicVolume = 0;
  // The menu's music fading out before the icon kit's comes in, and the icon kit's fading out on leaving
  float garageMusicFade = 0;

  // ! --- The pause menu --- !
  // MiSide's pause: a big title, a small subtitle, a column of see-through rows on the left with Quit apart under
  // them, the level's name in a long bar on the right, the level blurred and tinted behind. Points of the 569×320
  // screen, measured on MiSide's (2560×1440); the bar's right end keeps its distance from the screen's right edge
  bool pauseEnabled = false;
  float pauseLeft = 0;
  float pauseTitleLeft = 0;
  float pauseTitleCapTop = 0;
  float pauseTitleScale = 0;
  cocos2d::ccColor3B pauseTitleColor;
  float pauseSubtitleCapTop = 0;
  float pauseSubtitleScale = 0;
  cocos2d::ccColor3B pauseSubtitleColor;
  // The rows: the first one's top edge, one every `pause-row-pitch` points down; Quit `pause-exit-gap` further
  float pauseFirstRowTop = 0;
  float pauseRowPitch = 0;
  float pauseRowWidth = 0;
  float pauseRowHeight = 0;
  float pauseExitGap = 0;
  cocos2d::ccColor3B pauseRowColor;
  GLubyte pauseRowOpacity = 0;
  cocos2d::ccColor3B pauseRowHoverColor;
  GLubyte pauseRowHoverOpacity = 0;
  cocos2d::ccColor3B pauseRowPressedColor;
  GLubyte pauseRowPressedOpacity = 0;
  cocos2d::ccColor3B pauseTextColor;
  cocos2d::ccColor3B pauseTextHoverColor;
  float pauseTextScale = 0;
  float pauseTextOffset = 0;
  float pauseTextPadding = 0;
  bool pauseShineEnabled = false;
  // The volume sliders: the track's left edge from the row's, its size; the fill and the knob with the value on it
  float pauseSliderLeft = 0;
  float pauseSliderWidth = 0;
  float pauseSliderHeight = 0;
  cocos2d::ccColor3B pauseSliderTrackColor;
  cocos2d::ccColor3B pauseSliderFillColor;
  cocos2d::ccColor3B pauseSliderKnobColor;
  float pauseSliderKnobWidth = 0;
  cocos2d::ccColor3B pauseSliderValueColor;
  float pauseSliderValueScale = 0;
  // One arrow key press, as a share of the whole
  float pauseSliderStep = 0;
  // 0..100: a reset button next to the slider sets this; -1: no button (GD's own defaults aren't known for sure)
  float pauseMusicReset = 0;
  float pauseSfxReset = 0;
  float pauseResetSize = 0;
  cocos2d::ccColor3B pauseResetColor;
  // The level's bar and its numbers under it
  float pauseBarLeft = 0;
  float pauseBarRightMargin = 0;
  cocos2d::ccColor3B pauseBarColor;
  GLubyte pauseBarOpacity = 0;
  cocos2d::ccColor3B pauseBarBorderColor;
  GLubyte pauseBarBorderOpacity = 0;
  float pauseBarTextScale = 0;
  cocos2d::ccColor3B pauseBarTextColor;
  float pauseStatsScale = 0;
  float pauseStatsHeight = 0;
  float pauseStatsGap = 0;
  cocos2d::ccColor3B pauseStatsLabelColor;
  cocos2d::ccColor3B pauseStatsValueColor;
  GLubyte pauseStatsOpacity = 0;
  // The level behind, blurred once when the pause opens: at 1/downsample of the window's pixels (2, 4 or 8), a
  // Gaussian of this many points (its sigma) on the screen
  bool pauseBlurEnabled = false;
  float pauseBlurDownsample = 0;
  float pauseBlurRadius = 0;
  // Saves the captured level and the blurred picture as PNGs in the mod's config folder, each pause (to check the
  // capture; reads them back from the GPU, so off normally)
  bool pauseBlurDebug = false;
  // Over it: a pink-lilac tint, and a gradient from the left for the menu to read on
  cocos2d::ccColor3B pauseTintColor;
  GLubyte pauseTintOpacity = 0;
  cocos2d::ccColor3B pauseGradientColor;
  GLubyte pauseGradientOpacity = 0;
  // Where the gradient has faded out, as a share of the screen's width
  float pauseGradientWidth = 0;
  // With no blur (off, or it can't be made here): only this darkening
  cocos2d::ccColor3B pauseDimColor;
  GLubyte pauseDimOpacity = 0;
  // The pause's own ambient: a looping sound of its own, faded in and out. Off until a file is given: a path, or a
  // file name in the mod's config folder
  bool pauseAmbientEnabled = false;
  std::string pauseAmbientPath;
  float pauseAmbientVolume = 0;
  float pauseAmbientFade = 0;
  // Instead of the ambient, near the personal best: from `pause-stress-window` points under the best at the attempt's
  // start, a bit louder past it (`pause-stress-boost`). Not in practice or with a start position, not on a completed
  // level, not with a best under `pause-stress-min-best`
  bool pauseStressEnabled = false;
  std::string pauseStressPath;
  float pauseStressVolume = 0;
  float pauseStressWindow = 0;
  float pauseStressMinBest = 0;
  float pauseStressBoost = 0;
  // The menus the mods put their pause buttons in: hidden in the pause, shown as they are under More
  bool pauseExtraEnabled = false;
  std::string pauseExtraMenus;
  // The list's states (the confirmation, More) fading into each other
  float pauseDuration = 0;

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
  // Points per pixel of her 960×1080 canvas: 320 / 1080 makes it as high as the reference screen. The close-up is
  // in the render itself, nothing zooms it further
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
  // In her texture's pixels. Her frames have wide transparent sides (68 px at least, more than the line reaches into
  // the frame next to it); at their top and bottom edges the atlas repeats the edge, so no line shows there
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
