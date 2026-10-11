#include "Theme.hpp"

#include <Geode/utils/file.hpp>

using namespace geode::prelude;

namespace
{
  Theme s_theme;

  Theme defaults()
  {
    Theme theme;

    // ! --- Colors --- !
    // MiSide's: nearly black on the left, a deep burgundy towards the right
    theme.backdropLeft = ccc3(0x0a, 0x03, 0x08);
    theme.backdropMiddle = ccc3(0x26, 0x0a, 0x1c);
    theme.backdropRight = ccc3(0x3a, 0x0e, 0x2c);
    theme.rowColor = ccc3(0x6e, 0x3c, 0x78);
    theme.rowHoverColor = ccc3(0xc2, 0x5a, 0xa6);
    theme.rowPressedColor = ccc3(0xd9, 0x47, 0x9f);
    theme.rowDisabledColor = ccc3(0x35, 0x10, 0x2e);
    theme.itemColor = ccc3(0xf4, 0xc9, 0xea);
    // Lighter than the items' color, still pink: the shine's white shows on it
    theme.itemHoverColor = ccc3(0xff, 0xd3, 0xec);
    theme.itemPressedColor = ccc3(0xff, 0xff, 0xff);
    theme.subtitleColor = ccc3(0xff, 0xff, 0xff);
    theme.shadowColor = ccc3(0x2a, 0x0b, 0x26);
    theme.accentColor = ccc3(0xff, 0x6f, 0x91);
    theme.backgroundTint = ccc3(0xa0, 0x62, 0x8f);
    theme.darkenColor = ccc3(0x15, 0x05, 0x1a);
    theme.leftShadeColor = ccc3(0x08, 0x05, 0x0d);
    // A soft purple light behind Mita
    theme.glowColor = ccc3(0xa0, 0x1a, 0x80);
    theme.fadeColor = ccc3(0x09, 0x04, 0x0d);
    theme.trayColor = ccc3(0x1a, 0x10, 0x20);
    theme.slotColor = ccc3(0xff, 0xff, 0xff);
    theme.cardColor = ccc3(0x16, 0x0a, 0x1d);
    theme.avatarColor = ccc3(0x2e, 0x1b, 0x34);
    theme.nameColor = ccc3(0xf4, 0xc9, 0xea);
    theme.popupColor = ccc3(0x22, 0x14, 0x28);
    theme.rowOpacity = 150;
    theme.rowHoverOpacity = 225;
    theme.rowPressedOpacity = 240;
    theme.rowDisabledOpacity = 100;
    theme.itemOpacity = 255;
    theme.itemHoverOpacity = 255;
    theme.shadowOpacity = 110;
    theme.darkenOpacity = 115;
    theme.leftShadeOpacity = 190;
    theme.glowOpacity = 105;
    theme.characterOpacity = 255;
    theme.fadeOpacity = 200;
    theme.trayOpacity = 80;
    theme.slotOpacity = 0;
    theme.cardOpacity = 90;
    theme.avatarOpacity = 230;
    theme.popupOpacity = 240;

    // ! --- Fonts --- !
    theme.font = "miside-font.fnt"_spr;
    theme.titleFont = "miside-title.fnt"_spr;
    theme.fallbackFont = "Arial";
    theme.fallbackFontSize = 24;
    // Capitals about 8.5 pt high: the font's are 11.3 pt at scale 1
    theme.itemScale = 0.75f;
    theme.itemTextOffset = -1.2f;
    // Capitals about 10 pt high
    theme.subtitleScale = 0.62f;
    theme.nameScale = 0.5f;
    theme.moreScale = 0.9f;
    theme.popupTitleScale = 0.9f;
    theme.shadowSteps = 2;
    theme.shadowOffset = 0.6f;

    // ! --- Sprites and sounds --- !
    // No pictures: the backdrop gradient with the pattern, as MiSide's menus
    theme.layers = {};
    theme.rowSprite = "row-plate.png"_spr;
    theme.logoSprite = "miside-like-gd-logo.png"_spr;
    theme.glowSprite = "glow-soft.png"_spr;
    theme.characterSprite = "";
    theme.accentSprite = "item-accent.png"_spr;
    theme.traySprite = "tray-panel.png"_spr;
    theme.slotSprite = "tray-slot.png"_spr;
    theme.avatarSprite = "avatar-circle.png"_spr;
    // MiSide's: Table Enter on hovering a button (with its shine), Table Click on pressing it
    theme.hoverSound = "miside-table-enter.flac"_spr;
    theme.clickSound = "miside-table-click.flac"_spr;
    theme.toggleSound = "miside-table-toggle.flac"_spr;
    theme.tabSound = "miside-switch-category.flac"_spr;
    theme.moreOpenSound = "miside-switch-open.flac"_spr;
    theme.moreCloseSound = "miside-switch-close.flac"_spr;
    theme.outfitSound = "miside-gift-poof.flac"_spr;
    theme.achievementSound = "miside-achievement.flac"_spr;
    theme.winSound = "miside-win.flac"_spr;
    theme.pauseStartSound = "miside-startpause.flac"_spr;
    // It's quiet in the file (peaks at -34 dB)
    theme.pauseStartVolume = 1;
    theme.soundVolume = 0.5f;

    // ! --- Hover and the Exit button --- !
    theme.shineEnabled = true;
    theme.shineTint = ccc3(0xff, 0xe6, 0xf4);
    theme.shineCoreColor = ccc3(0xff, 0xff, 0xff);
    // A little slower than 0.2 s (the glow's longer reach makes the path longer too)
    theme.shineDuration = 0.28f;
    theme.shineBandWidth = 0.7f;
    // Over 1: the band's opaque part is wider
    theme.shineIntensity = 0.95f;
    theme.shineCore = 0.6f;
    theme.shineSlant = 0;
    theme.shineFadeOut = 0.12f;
    theme.shineFreeze = -1;
    // Like MiSide's: white by the letters, a wide soft halo
    theme.shineGlowRadius = 16;
    theme.shineGlowIntensity = 3.0f;
    theme.shineGlowInnerRadius = 2.5f;
    theme.shineGlowInnerIntensity = 1.1f;
    theme.shineGlowSpread = 2.0f;
    theme.shineGlowColor = ccc3(0xff, 0xff, 0xff);
    theme.rowGlowColor = ccc3(0xff, 0x5f, 0xb4);
    theme.rowGlowOpacity = 110;
    theme.rowGlowSpread = 7;
    theme.exitDodgeEnabled = true;
    theme.exitDodgeRadius = 55;
    theme.exitDodgeMax = 11;
    theme.exitDodgeVertical = 0.35f;
    theme.exitDodgeStretch = 0.04f;
    theme.exitDodgeTilt = 3;
    theme.exitDodgeStiffness = 170;
    theme.exitDodgeDamping = 0.55f;

    // ! --- The menu --- !
    theme.columnLeft = 59;
    // Over the subtitle (its top at 229), left of Mita's hair (from about x 305 at that height): 220 x 36 points
    theme.logoLeft = 59;
    theme.logoBottom = 236;
    theme.logoWidth = 220;
    theme.subtitleTop = 229;
    theme.firstRowY = 195;
    theme.rowPitch = 17;
    theme.rowWidth = 148;
    theme.rowHeight = 15;
    theme.textPadding = 5;
    theme.exitWidth = 59;
    theme.exitY = 37;
    theme.columnClearance = 6;
    theme.accentEnabled = false;
    theme.accentWidth = 0;
    theme.accentGap = 0;
    theme.hoverShift = 0;
    theme.pressedScale = 1.f;

    // ! --- The quit confirmation --- !
    // Measured on MiSide's screen (2560×1440): title capitals 8.7 pt, the buttons 14.9 pt high every 16.4 pt,
    // 5.1 pt under the title's line, their text 6.7 pt, both as heavy as the ExtraBold. Moved down 5.6 pt together,
    // so the title takes the subtitle's place under our logo
    theme.confirmTitleLeft = 60.2f;
    theme.confirmTitleCapTop = 227.5f;
    // The title font's capitals are 17.5 pt at scale 1
    theme.confirmTitleScale = 0.5f;
    theme.confirmTitleColor = ccc3(0xff, 0xff, 0xff);
    theme.confirmTitleShadowColor = ccc3(0x05, 0x05, 0x0c);
    theme.confirmTitleShadowOpacity = 255;
    theme.confirmTitleShadowOffset = 0.65f;
    theme.confirmRowLeft = 59;
    theme.confirmFirstRowTop = 213.7f;
    theme.confirmRowPitch = 16.4f;
    theme.confirmRowWidth = 148.4f;
    theme.confirmRowHeight = 14.9f;
    // #9F60A1 at 39 %: what the plate does to the background behind it there
    theme.confirmRowColor = ccc3(0x9f, 0x60, 0xa1);
    theme.confirmRowOpacity = 100;
    theme.confirmTextColor = ccc3(0xf6, 0xa0, 0xfd);
    theme.confirmTextFont = "miside-title.fnt"_spr;
    // The title font's capitals are 17.5 pt at scale 1
    theme.confirmTextScale = 0.38f;
    // The capitals centred in the plate: their middle is 2.82 pt over the label's at scale 1
    theme.confirmTextOffset = -1.07f;
    theme.confirmTextPadding = 3.6f;
    theme.confirmDuration = 0.2f;

    // ! --- Music --- !
    // Suggested from the tracks' names, not how MiSide uses them. The other menu tracks: "miside-music-menu.ogg",
    // "miside-menu-alternative.ogg"
    theme.menuMusicEnabled = true;
    theme.menuMusicPath = "miside-music-menu-update.ogg";
    theme.menuMusicVolume = 1;
    theme.garageMusicEnabled = true;
    theme.garageMusicPath = "miside-ambient-chibi-room.ogg";
    theme.garageMusicVolume = 0.85f;
    theme.garageMusicFade = 0.5f;

    // ! --- The pause menu --- !
    // Measured on MiSide's pause (2560×1440): title capitals 15.3 pt, subtitle 6.4 pt, rows 148×14.9 pt every 16.2,
    // their text 8.2 pt, Quit 7.3 pt further down; the bar from 195.3 to 524.4 pt at the first row's height
    theme.pauseEnabled = true;
    theme.pauseLeft = 44.4f;
    theme.pauseTitleLeft = 60.9f;
    theme.pauseTitleCapTop = 256.2f;
    // The title font's capitals are 17.5 pt at scale 1
    theme.pauseTitleScale = 0.875f;
    theme.pauseTitleColor = ccc3(0xff, 0xff, 0xff);
    theme.pauseSubtitleCapTop = 234.9f;
    theme.pauseSubtitleScale = 0.37f;
    theme.pauseSubtitleColor = ccc3(0xff, 0xff, 0xff);
    theme.pauseFirstRowTop = 225.3f;
    theme.pauseRowPitch = 16.2f;
    theme.pauseRowWidth = 148;
    theme.pauseRowHeight = 14.9f;
    theme.pauseExitGap = 7.3f;
    // A light pink over the tinted level: about 40 lighter than what's around, as MiSide's
    theme.pauseRowColor = ccc3(0xff, 0xc8, 0xf0);
    theme.pauseRowOpacity = 85;
    theme.pauseRowHoverColor = ccc3(0xff, 0xd8, 0xf6);
    theme.pauseRowHoverOpacity = 150;
    theme.pauseRowPressedColor = ccc3(0xff, 0xe8, 0xfa);
    theme.pauseRowPressedOpacity = 190;
    theme.pauseTextColor = ccc3(0xf6, 0xa0, 0xff);
    theme.pauseTextHoverColor = ccc3(0xff, 0xc4, 0xff);
    theme.pauseTextScale = 0.47f;
    // The capitals' middle is 2.82 pt over the label's at scale 1
    theme.pauseTextOffset = -1.33f;
    theme.pauseTextPadding = 3.6f;
    theme.pauseShineEnabled = true;
    theme.pauseSliderLeft = 73.8f;
    theme.pauseSliderWidth = 59.6f;
    theme.pauseSliderHeight = 7.3f;
    theme.pauseSliderTrackColor = ccc3(0x25, 0x00, 0x26);
    theme.pauseSliderFillColor = ccc3(0xff, 0x5a, 0xf3);
    theme.pauseSliderKnobColor = ccc3(0xff, 0xff, 0xff);
    // MiSide's is 11.8 for its narrow digits; ours need room for 100
    theme.pauseSliderKnobWidth = 13.5f;
    theme.pauseSliderValueColor = ccc3(0xf1, 0x00, 0xc1);
    theme.pauseSliderValueScale = 0.3f;
    theme.pauseSliderStep = 0.05f;
    theme.pauseMusicReset = -1;
    theme.pauseSfxReset = -1;
    theme.pauseResetSize = 11.3f;
    theme.pauseResetColor = ccc3(0xff, 0x8c, 0xf0);
    theme.pauseBarLeft = 195.3f;
    theme.pauseBarRightMargin = 44.6f;
    theme.pauseBarColor = ccc3(0x55, 0x28, 0x56);
    theme.pauseBarOpacity = 235;
    theme.pauseBarBorderColor = ccc3(0xff, 0xff, 0xff);
    theme.pauseBarBorderOpacity = 45;
    theme.pauseBarTextScale = 0.47f;
    theme.pauseBarTextColor = ccc3(0xff, 0xff, 0xff);
    theme.pauseStatsScale = 0.3f;
    theme.pauseStatsHeight = 10;
    theme.pauseStatsGap = 2.5f;
    theme.pauseStatsLabelColor = ccc3(0xf6, 0xa0, 0xff);
    theme.pauseStatsValueColor = ccc3(0xff, 0xff, 0xff);
    theme.pauseStatsOpacity = 200;
    theme.pauseBlurEnabled = true;
    theme.pauseBlurDownsample = 4;
    // The level's blocks, spikes and ground still there as soft shapes (7 pt and a strong fuchsia tint left Back On
    // Track a flat pink)
    theme.pauseBlurRadius = 3;
    // A muted lilac over it, not a fuchsia fill
    theme.pauseTintColor = ccc3(0xb4, 0x8c, 0xc4);
    theme.pauseTintOpacity = 55;
    theme.pauseGradientColor = ccc3(0x6e, 0x45, 0x80);
    theme.pauseGradientOpacity = 100;
    theme.pauseGradientWidth = 0.5f;
    theme.pauseBlurDebug = false;
    theme.pauseDimColor = ccc3(0x1a, 0x0a, 0x1e);
    theme.pauseDimOpacity = 170;
    // MiSide's pause ambient, a seamless loop
    theme.pauseAmbientEnabled = true;
    theme.pauseAmbientPath = "miside-ambientpause.flac";
    theme.pauseAmbientVolume = 0.6f;
    theme.pauseStressEnabled = true;
    theme.pauseStressPath = "miside-stress.flac";
    // It's louder in the file than the ambient (peaks at -2.5 dB against -19 dB)
    theme.pauseStressVolume = 0.3f;
    theme.pauseStressWindow = 5;
    theme.pauseStressMinBest = 20;
    theme.pauseStressBoost = 1.25f;
    theme.pauseAmbientFade = 1;
    theme.pauseExtraEnabled = true;
    theme.pauseExtraMenus = "left-button-menu,right-button-menu,bottom-button-menu";
    theme.pauseDuration = 0.2f;

    // ! --- The intro --- !
    // AIHASTO's intro, from a recording of MiSide's start (21.8-29.3 s), 1280×720 at 30 fps
    theme.introEnabled = true;
    theme.introFps = 30;
    theme.introBackground = ccc3(0x0f, 0x0e, 0x13);
    theme.introVolume = 0.8f;
    theme.introFadeOut = 0.4f;

    // ! --- The loading screen --- !
    // Measured on MiSide's loading screen: the middle #670A6B, the edges #260237; LOADING... capitals 3.6 pt at
    // 25.9 pt from the left and 27.5 from the bottom; the bar 2.4 pt high from 25.9 to 136 pt, 19 pt up
    theme.loadingEnabled = true;
    theme.loadingEdgeColor = ccc3(0x24, 0x02, 0x36);
    theme.loadingGlowColor = ccc3(0x8a, 0x10, 0x90);
    theme.loadingGlowSize = 1.5f;
    theme.loadingPatternColor = ccc3(0x14, 0x00, 0x1a);
    theme.loadingPatternOpacity = 70;
    theme.loadingLeft = 25.9f;
    theme.loadingCaptionCapTop = 27.5f;
    theme.loadingCaptionCap = 3.64f;
    theme.loadingCaptionColor = ccc3(0xf5, 0x80, 0xa0);
    theme.loadingBarY = 16.5f;
    theme.loadingBarWidth = 110;
    theme.loadingBarHeight = 2.4f;
    theme.loadingTrackColor = ccc3(0x26, 0x00, 0x26);
    theme.loadingTrackEdgeColor = ccc3(0x72, 0x3b, 0x61);
    theme.loadingFillStartColor = ccc3(0x66, 0x20, 0x71);
    theme.loadingFillEndColor = ccc3(0xc5, 0x61, 0x95);
    theme.loadingHeadColor = ccc3(0xff, 0xc8, 0xc0);
    theme.loadingTipY = 15.5f;
    theme.loadingLineY = 3.2f;
    theme.loadingLineHeight = 5.5f;
    theme.loadingTextGap = 8;
    theme.loadingPhraseDuration = 2.4f;
    theme.loadingPhraseFade = 0.2f;
    theme.loadingTipCap = 3;
    theme.loadingTipColor = ccc3(0xff, 0xd0, 0xe6);

    // ! --- The official levels --- !
    // Measured on MiSide's load screen: title capitals 8.7 pt, rows 136×14.9 pt every 16.2 from 275.8, 13 in view,
    // the scroll bar at 202, BACK at 56, the floppy from 252 to 461 and 30 to 272
    theme.levelsEnabled = true;
    theme.levelsTitleLeft = 59.8f;
    theme.levelsTitleCapTop = 292.4f;
    theme.levelsRowLeft = 62;
    theme.levelsRowWidth = 136;
    theme.levelsRowHeight = 14.9f;
    theme.levelsRowPitch = 16.2f;
    theme.levelsFirstRowTop = 275.8f;
    theme.levelsVisibleRows = 13;
    theme.levelsRowColor = ccc3(0x63, 0x17, 0x6f);
    theme.levelsRowOpacity = 240;
    theme.levelsRowChosenColor = ccc3(0x82, 0x14, 0x8c);
    theme.levelsRowHoverColor = ccc3(0x92, 0x2c, 0x9c);
    theme.levelsTextColor = ccc3(0xf6, 0xa0, 0xfd);
    theme.levelsTextChosenColor = ccc3(0xff, 0xd2, 0xff);
    theme.levelsScrollLeft = 202;
    theme.levelsScrollWidth = 5;
    theme.levelsScrollTrackColor = ccc3(0x68, 0x00, 0x84);
    theme.levelsScrollThumbColor = ccc3(0xff, 0xff, 0xff);
    theme.levelsBackTop = 56;
    theme.levelsBackWidth = 100;
    theme.levelsMoreWidth = 44;
    theme.levelsCardLeft = 252;
    theme.levelsCardRight = 461;
    theme.levelsCardTop = 272;
    theme.levelsCardBottom = 30;
    theme.levelsCardColor = ccc3(0xfe, 0xde, 0xf2);
    theme.levelsCardFrameColor = ccc3(0xfc, 0x4c, 0xa0);
    theme.levelsCardShadowColor = ccc3(0x3a, 0x0b, 0x37);
    theme.levelsCardLabelColor = ccc3(0xff, 0xff, 0xff);
    theme.levelsCardTextColor = ccc3(0x5a, 0x12, 0x60);
    theme.levelsCardAccentColor = ccc3(0xff, 0x4f, 0xa8);
    theme.levelsCardHoleColor = ccc3(0x49, 0x0e, 0x56);
    theme.levelsPlayTextColor = ccc3(0xff, 0xff, 0xff);
    theme.levelsPreviewEmptyColor = ccc3(0x2a, 0x06, 0x24);
    theme.levelsPreviewFade = 0.25f;
    theme.levelsPreviewWidth = 640;

    // ! --- The achievements --- !
    // Measured on MiSide's: title capitals 9.1 pt from 59.7 pt, 12.8 from the top; rows 137×23.3 pt every 26.6 from
    // 25.6 pt down, 10 in view, the icon 19.5 pt in a white frame; the scroll bar at 202, 4.5 wide; BACK 14 pt high,
    // 26.4 pt up, as wide as the rows and the bar
    theme.achievementsEnabled = true;
    theme.achievementsTitleLeft = 59.7f;
    theme.achievementsTitleCapTop = 307.2f;
    theme.achievementsCounterScale = 0.32f;
    theme.achievementsCounterColor = ccc3(0xd8, 0x9c, 0xd8);
    theme.achievementsListLeft = 61.2f;
    theme.achievementsListTop = 294.4f;
    theme.achievementsListBottom = 30;
    theme.achievementsRowWidth = 137.1f;
    theme.achievementsRowHeight = 23.3f;
    theme.achievementsRowGap = 3.3f;
    theme.achievementsRowColor = ccc3(0x6a, 0x2a, 0x84);
    theme.achievementsRowOpacity = 205;
    theme.achievementsRowHoverColor = ccc3(0x92, 0x3a, 0xac);
    theme.achievementsRowHoverOpacity = 240;
    theme.achievementsRowLockedColor = ccc3(0x3c, 0x16, 0x4a);
    theme.achievementsRowLockedOpacity = 175;
    theme.achievementsTitleColor = ccc3(0xff, 0xff, 0xff);
    theme.achievementsTitleLockedColor = ccc3(0xc0, 0x96, 0xc8);
    theme.achievementsTitleLockedOpacity = 200;
    theme.achievementsTitleScale = 0.36f;
    theme.achievementsTextColor = ccc3(0xf2, 0xd2, 0xf2);
    theme.achievementsTextScale = 0.36f;
    theme.achievementsStatusColor = ccc3(0xff, 0x9e, 0xd8);
    theme.achievementsIconSize = 19.5f;
    theme.achievementsIconFrameColor = ccc3(0xff, 0xff, 0xff);
    theme.achievementsIconBackColor = ccc3(0x2a, 0x0e, 0x30);
    theme.achievementsCheckColor = ccc3(0xff, 0x8f, 0xd0);
    theme.achievementsProgressTrackColor = ccc3(0x26, 0x08, 0x2c);
    theme.achievementsProgressFillColor = ccc3(0xff, 0x6f, 0xb8);
    theme.achievementsScrollLeft = 202;
    theme.achievementsScrollWidth = 4.5f;
    theme.achievementsScrollTrackColor = ccc3(0x68, 0x00, 0x84);
    theme.achievementsScrollThumbColor = ccc3(0xff, 0xff, 0xff);
    theme.achievementsBackTop = 26.4f;
    theme.achievementsBackWidth = 145.3f;
    theme.achievementsExpandDuration = 0.18f;
    theme.achievementsScrollSmoothing = 0.08f;

    // ! --- Decoration over the backdrop --- !
    // Faint: a pink line on the dark, never a row of bright marks
    theme.patternSprite = "pattern-tile.png"_spr;
    theme.patternColor = ccc3(0xe0, 0x50, 0x9e);
    theme.patternOpacity = 38;
    theme.patternTile = 256;
    theme.patternDepth = 0.3f;
    theme.vignetteSprite = "vignette-soft.png"_spr;
    theme.vignetteColor = ccc3(0x12, 0x03, 0x0e);
    theme.vignetteOpacity = 180;
    // Soft glows, so only their middle reads: big and bright enough to be seen (1.2-3 pt at 150 hardly were)
    theme.particleCount = 48;
    // MiSide's motes: magenta streaks, 3-4 times longer than wide, leaning 40 degrees
    theme.particleColor = ccc3(0xff, 0x10, 0xf0);
    theme.particleOpacity = 235;
    theme.particleSizeMin = 3.f;
    theme.particleSizeMax = 7.f;
    // Points a second, along the way they point
    theme.particleSpeed = 38;
    theme.particleLife = 8;
    theme.particleStretch = 3.5f;
    theme.particleAngle = -40;
    theme.particleTurn = 1.6f;
    theme.particleFlock = 1;
    theme.particleFlockRadius = 60;

    // ! --- The background --- !
    theme.backdropMiddleX = 0.45f;
    theme.backgroundFocusX = 0.5f;
    theme.backgroundFocusY = 0.5f;
    theme.leftShadeWidth = 260;
    theme.glowX = 0.72f;
    theme.glowY = 0.5f;
    theme.glowWidth = 460;
    theme.glowHeight = 420;
    theme.characterX = 0.71f;
    theme.characterHeight = 1.f;
    theme.characterDepth = 0.6f;
    theme.parallaxRange = 2;

    // ! --- Mita --- !
    theme.mitaEnabled = true;
    // The close-up set's placement: anchor at her canvas' bottom middle, the canvas as high as the screen
    theme.mitaX = 414.5f;
    theme.mitaY = 0;
    theme.mitaScale = 320.f / 1080.f;
    theme.mitaFps = 24;
    theme.mitaParallaxEnabled = false;
    theme.mitaParallaxMaxX = 3;
    theme.mitaParallaxMaxY = 2;
    theme.mitaFreezeFrame = -1;
    theme.mitaOutlineEnabled = true;
    theme.mitaOutlineColor = ccc3(0xff, 0x4f, 0xa3);
    theme.mitaOutlineOpacity = 210;
    // About 2.4 points on the screen, as before at the old set's scale
    theme.mitaOutlineWidth = 8;

    // ! --- The tray and the profile --- !
    theme.trayMargin = 8;
    theme.trayPadding = 4;
    theme.trayGap = 3;
    theme.slotSize = 13;
    theme.slotPadding = 2;
    theme.trayMaxWidth = 0.6f;
    theme.cardLeft = 10;
    theme.cardBottom = 8;
    theme.cardPadding = 3;
    theme.cardGap = 4;
    theme.avatarSize = 14;
    theme.avatarIcon = 0.7f;
    theme.cardPressScale = 1.04f;
    theme.popupSlotSize = 30;
    theme.popupColumns = 4;
    theme.popupGap = 8;
    theme.popupPadding = 14;

    // ! --- Timings --- !
    theme.hoverDuration = 0.08f;
    theme.introDuration = 0.18f;
    theme.introStagger = 0.015f;
    theme.introShift = 0;
    theme.fadeDuration = 0.15f;
    theme.fadeBackDuration = 0.15f;
    theme.parallaxSmoothing = 0.12f;
    return theme;
  }

  // Every field with its key in theme.json
  template <class F>
  void fields(Theme& theme, F&& field)
  {
    field("backdrop-left", theme.backdropLeft);
    field("backdrop-middle", theme.backdropMiddle);
    field("backdrop-right", theme.backdropRight);
    field("row-color", theme.rowColor);
    field("row-hover-color", theme.rowHoverColor);
    field("row-pressed-color", theme.rowPressedColor);
    field("row-disabled-color", theme.rowDisabledColor);
    field("item-color", theme.itemColor);
    field("item-hover-color", theme.itemHoverColor);
    field("item-pressed-color", theme.itemPressedColor);
    field("subtitle-color", theme.subtitleColor);
    field("shadow-color", theme.shadowColor);
    field("accent-color", theme.accentColor);
    field("background-tint", theme.backgroundTint);
    field("background-darken-color", theme.darkenColor);
    field("left-shade-color", theme.leftShadeColor);
    field("glow-color", theme.glowColor);
    field("fade-color", theme.fadeColor);
    field("tray-color", theme.trayColor);
    field("slot-color", theme.slotColor);
    field("card-color", theme.cardColor);
    field("avatar-color", theme.avatarColor);
    field("name-color", theme.nameColor);
    field("popup-color", theme.popupColor);
    field("row-opacity", theme.rowOpacity);
    field("row-hover-opacity", theme.rowHoverOpacity);
    field("row-pressed-opacity", theme.rowPressedOpacity);
    field("row-disabled-opacity", theme.rowDisabledOpacity);
    field("item-opacity", theme.itemOpacity);
    field("item-hover-opacity", theme.itemHoverOpacity);
    field("shadow-opacity", theme.shadowOpacity);
    field("background-darken-opacity", theme.darkenOpacity);
    field("left-shade-opacity", theme.leftShadeOpacity);
    field("glow-opacity", theme.glowOpacity);
    field("character-opacity", theme.characterOpacity);
    field("fade-opacity", theme.fadeOpacity);
    field("tray-opacity", theme.trayOpacity);
    field("slot-opacity", theme.slotOpacity);
    field("card-opacity", theme.cardOpacity);
    field("avatar-opacity", theme.avatarOpacity);
    field("popup-opacity", theme.popupOpacity);

    field("font", theme.font);
    field("title-font", theme.titleFont);
    field("fallback-font", theme.fallbackFont);
    field("fallback-font-size", theme.fallbackFontSize);
    field("item-scale", theme.itemScale);
    field("item-text-offset", theme.itemTextOffset);
    field("subtitle-scale", theme.subtitleScale);
    field("name-scale", theme.nameScale);
    field("more-scale", theme.moreScale);
    field("popup-title-scale", theme.popupTitleScale);
    field("shadow-steps", theme.shadowSteps);
    field("shadow-offset", theme.shadowOffset);

    field("layers", theme.layers);
    field("row-sprite", theme.rowSprite);
    field("logo-sprite", theme.logoSprite);
    field("glow-sprite", theme.glowSprite);
    field("character-sprite", theme.characterSprite);
    field("accent-sprite", theme.accentSprite);
    field("tray-sprite", theme.traySprite);
    field("slot-sprite", theme.slotSprite);
    field("avatar-sprite", theme.avatarSprite);
    field("hover-sound", theme.hoverSound);
    field("click-sound", theme.clickSound);
    field("sound-volume", theme.soundVolume);

    field("shine-enabled", theme.shineEnabled);
    field("shine-tint", theme.shineTint);
    field("shine-core-color", theme.shineCoreColor);
    field("shine-duration", theme.shineDuration);
    field("shine-band-width", theme.shineBandWidth);
    field("shine-intensity", theme.shineIntensity);
    field("shine-core", theme.shineCore);
    field("shine-slant", theme.shineSlant);
    field("shine-fade-out", theme.shineFadeOut);
    field("shine-freeze", theme.shineFreeze);
    field("shine-glow-radius", theme.shineGlowRadius);
    field("shine-glow-intensity", theme.shineGlowIntensity);
    field("shine-glow-inner-radius", theme.shineGlowInnerRadius);
    field("shine-glow-inner-intensity", theme.shineGlowInnerIntensity);
    field("shine-glow-spread", theme.shineGlowSpread);
    field("shine-glow-color", theme.shineGlowColor);
    field("row-glow-color", theme.rowGlowColor);
    field("row-glow-opacity", theme.rowGlowOpacity);
    field("row-glow-spread", theme.rowGlowSpread);
    field("exit-dodge-enabled", theme.exitDodgeEnabled);
    field("exit-dodge-radius", theme.exitDodgeRadius);
    field("exit-dodge-max", theme.exitDodgeMax);
    field("exit-dodge-vertical", theme.exitDodgeVertical);
    field("exit-dodge-stretch", theme.exitDodgeStretch);
    field("exit-dodge-tilt", theme.exitDodgeTilt);
    field("exit-dodge-stiffness", theme.exitDodgeStiffness);
    field("exit-dodge-damping", theme.exitDodgeDamping);

    field("column-left", theme.columnLeft);
    field("logo-left", theme.logoLeft);
    field("logo-bottom", theme.logoBottom);
    field("logo-width", theme.logoWidth);
    field("subtitle-top", theme.subtitleTop);
    field("first-row-y", theme.firstRowY);
    field("row-pitch", theme.rowPitch);
    field("row-width", theme.rowWidth);
    field("row-height", theme.rowHeight);
    field("text-padding", theme.textPadding);
    field("exit-width", theme.exitWidth);
    field("exit-y", theme.exitY);
    field("column-clearance", theme.columnClearance);
    field("accent-enabled", theme.accentEnabled);
    field("accent-width", theme.accentWidth);
    field("accent-gap", theme.accentGap);
    field("hover-shift", theme.hoverShift);
    field("pressed-scale", theme.pressedScale);

    field("confirm-title-left", theme.confirmTitleLeft);
    field("confirm-title-cap-top", theme.confirmTitleCapTop);
    field("confirm-title-scale", theme.confirmTitleScale);
    field("confirm-title-color", theme.confirmTitleColor);
    field("confirm-title-shadow-color", theme.confirmTitleShadowColor);
    field("confirm-title-shadow-opacity", theme.confirmTitleShadowOpacity);
    field("confirm-title-shadow-offset", theme.confirmTitleShadowOffset);
    field("confirm-row-left", theme.confirmRowLeft);
    field("confirm-first-row-top", theme.confirmFirstRowTop);
    field("confirm-row-pitch", theme.confirmRowPitch);
    field("confirm-row-width", theme.confirmRowWidth);
    field("confirm-row-height", theme.confirmRowHeight);
    field("confirm-row-color", theme.confirmRowColor);
    field("confirm-row-opacity", theme.confirmRowOpacity);
    field("confirm-text-color", theme.confirmTextColor);
    field("confirm-text-font", theme.confirmTextFont);
    field("confirm-text-scale", theme.confirmTextScale);
    field("confirm-text-offset", theme.confirmTextOffset);
    field("confirm-text-padding", theme.confirmTextPadding);
    field("confirm-duration", theme.confirmDuration);

    field("toggle-sound", theme.toggleSound);
    field("tab-sound", theme.tabSound);
    field("more-open-sound", theme.moreOpenSound);
    field("more-close-sound", theme.moreCloseSound);
    field("outfit-sound", theme.outfitSound);
    field("achievement-sound", theme.achievementSound);
    field("win-sound", theme.winSound);
    field("pause-start-sound", theme.pauseStartSound);
    field("pause-start-volume", theme.pauseStartVolume);
    field("pause-stress-enabled", theme.pauseStressEnabled);
    field("pause-stress-path", theme.pauseStressPath);
    field("pause-stress-volume", theme.pauseStressVolume);
    field("pause-stress-window", theme.pauseStressWindow);
    field("pause-stress-min-best", theme.pauseStressMinBest);
    field("pause-stress-boost", theme.pauseStressBoost);

    field("menu-music-enabled", theme.menuMusicEnabled);
    field("menu-music-path", theme.menuMusicPath);
    field("menu-music-volume", theme.menuMusicVolume);
    field("garage-music-enabled", theme.garageMusicEnabled);
    field("garage-music-path", theme.garageMusicPath);
    field("garage-music-volume", theme.garageMusicVolume);
    field("garage-music-fade", theme.garageMusicFade);

    field("pause-enabled", theme.pauseEnabled);
    field("pause-left", theme.pauseLeft);
    field("pause-title-left", theme.pauseTitleLeft);
    field("pause-title-cap-top", theme.pauseTitleCapTop);
    field("pause-title-scale", theme.pauseTitleScale);
    field("pause-title-color", theme.pauseTitleColor);
    field("pause-subtitle-cap-top", theme.pauseSubtitleCapTop);
    field("pause-subtitle-scale", theme.pauseSubtitleScale);
    field("pause-subtitle-color", theme.pauseSubtitleColor);
    field("pause-first-row-top", theme.pauseFirstRowTop);
    field("pause-row-pitch", theme.pauseRowPitch);
    field("pause-row-width", theme.pauseRowWidth);
    field("pause-row-height", theme.pauseRowHeight);
    field("pause-exit-gap", theme.pauseExitGap);
    field("pause-row-color", theme.pauseRowColor);
    field("pause-row-opacity", theme.pauseRowOpacity);
    field("pause-row-hover-color", theme.pauseRowHoverColor);
    field("pause-row-hover-opacity", theme.pauseRowHoverOpacity);
    field("pause-row-pressed-color", theme.pauseRowPressedColor);
    field("pause-row-pressed-opacity", theme.pauseRowPressedOpacity);
    field("pause-text-color", theme.pauseTextColor);
    field("pause-text-hover-color", theme.pauseTextHoverColor);
    field("pause-text-scale", theme.pauseTextScale);
    field("pause-text-offset", theme.pauseTextOffset);
    field("pause-text-padding", theme.pauseTextPadding);
    field("pause-shine-enabled", theme.pauseShineEnabled);
    field("pause-slider-left", theme.pauseSliderLeft);
    field("pause-slider-width", theme.pauseSliderWidth);
    field("pause-slider-height", theme.pauseSliderHeight);
    field("pause-slider-track-color", theme.pauseSliderTrackColor);
    field("pause-slider-fill-color", theme.pauseSliderFillColor);
    field("pause-slider-knob-color", theme.pauseSliderKnobColor);
    field("pause-slider-knob-width", theme.pauseSliderKnobWidth);
    field("pause-slider-value-color", theme.pauseSliderValueColor);
    field("pause-slider-value-scale", theme.pauseSliderValueScale);
    field("pause-slider-step", theme.pauseSliderStep);
    field("pause-music-reset", theme.pauseMusicReset);
    field("pause-sfx-reset", theme.pauseSfxReset);
    field("pause-reset-size", theme.pauseResetSize);
    field("pause-reset-color", theme.pauseResetColor);
    field("pause-bar-left", theme.pauseBarLeft);
    field("pause-bar-right-margin", theme.pauseBarRightMargin);
    field("pause-bar-color", theme.pauseBarColor);
    field("pause-bar-opacity", theme.pauseBarOpacity);
    field("pause-bar-border-color", theme.pauseBarBorderColor);
    field("pause-bar-border-opacity", theme.pauseBarBorderOpacity);
    field("pause-bar-text-scale", theme.pauseBarTextScale);
    field("pause-bar-text-color", theme.pauseBarTextColor);
    field("pause-stats-scale", theme.pauseStatsScale);
    field("pause-stats-height", theme.pauseStatsHeight);
    field("pause-stats-gap", theme.pauseStatsGap);
    field("pause-stats-label-color", theme.pauseStatsLabelColor);
    field("pause-stats-value-color", theme.pauseStatsValueColor);
    field("pause-stats-opacity", theme.pauseStatsOpacity);
    field("pause-blur-enabled", theme.pauseBlurEnabled);
    field("pause-blur-downsample", theme.pauseBlurDownsample);
    field("pause-blur-radius", theme.pauseBlurRadius);
    field("pause-blur-debug", theme.pauseBlurDebug);
    field("pause-tint-color", theme.pauseTintColor);
    field("pause-tint-opacity", theme.pauseTintOpacity);
    field("pause-gradient-color", theme.pauseGradientColor);
    field("pause-gradient-opacity", theme.pauseGradientOpacity);
    field("pause-gradient-width", theme.pauseGradientWidth);
    field("pause-dim-color", theme.pauseDimColor);
    field("pause-dim-opacity", theme.pauseDimOpacity);
    field("pause-ambient-enabled", theme.pauseAmbientEnabled);
    field("pause-ambient-path", theme.pauseAmbientPath);
    field("pause-ambient-volume", theme.pauseAmbientVolume);
    field("pause-ambient-fade", theme.pauseAmbientFade);
    field("pause-extra-enabled", theme.pauseExtraEnabled);
    field("pause-extra-menus", theme.pauseExtraMenus);
    field("pause-duration", theme.pauseDuration);

    field("intro-enabled", theme.introEnabled);
    field("intro-fps", theme.introFps);
    field("intro-background", theme.introBackground);
    field("intro-volume", theme.introVolume);
    field("intro-fade-out", theme.introFadeOut);
    field("loading-enabled", theme.loadingEnabled);
    field("loading-edge-color", theme.loadingEdgeColor);
    field("loading-glow-color", theme.loadingGlowColor);
    field("loading-glow-size", theme.loadingGlowSize);
    field("loading-pattern-color", theme.loadingPatternColor);
    field("loading-pattern-opacity", theme.loadingPatternOpacity);
    field("loading-left", theme.loadingLeft);
    field("loading-caption-cap-top", theme.loadingCaptionCapTop);
    field("loading-caption-cap", theme.loadingCaptionCap);
    field("loading-caption-color", theme.loadingCaptionColor);
    field("loading-bar-y", theme.loadingBarY);
    field("loading-bar-width", theme.loadingBarWidth);
    field("loading-bar-height", theme.loadingBarHeight);
    field("loading-track-color", theme.loadingTrackColor);
    field("loading-track-edge-color", theme.loadingTrackEdgeColor);
    field("loading-fill-start-color", theme.loadingFillStartColor);
    field("loading-fill-end-color", theme.loadingFillEndColor);
    field("loading-head-color", theme.loadingHeadColor);
    field("loading-tip-y", theme.loadingTipY);
    field("loading-line-y", theme.loadingLineY);
    field("loading-line-height", theme.loadingLineHeight);
    field("loading-text-gap", theme.loadingTextGap);
    field("loading-phrase-duration", theme.loadingPhraseDuration);
    field("loading-phrase-fade", theme.loadingPhraseFade);
    field("loading-tip-cap", theme.loadingTipCap);
    field("loading-tip-color", theme.loadingTipColor);

    field("levels-enabled", theme.levelsEnabled);
    field("levels-title-left", theme.levelsTitleLeft);
    field("levels-title-cap-top", theme.levelsTitleCapTop);
    field("levels-row-left", theme.levelsRowLeft);
    field("levels-row-width", theme.levelsRowWidth);
    field("levels-row-height", theme.levelsRowHeight);
    field("levels-row-pitch", theme.levelsRowPitch);
    field("levels-first-row-top", theme.levelsFirstRowTop);
    field("levels-visible-rows", theme.levelsVisibleRows);
    field("levels-row-color", theme.levelsRowColor);
    field("levels-row-opacity", theme.levelsRowOpacity);
    field("levels-row-chosen-color", theme.levelsRowChosenColor);
    field("levels-row-hover-color", theme.levelsRowHoverColor);
    field("levels-text-color", theme.levelsTextColor);
    field("levels-text-chosen-color", theme.levelsTextChosenColor);
    field("levels-scroll-left", theme.levelsScrollLeft);
    field("levels-scroll-width", theme.levelsScrollWidth);
    field("levels-scroll-track-color", theme.levelsScrollTrackColor);
    field("levels-scroll-thumb-color", theme.levelsScrollThumbColor);
    field("levels-back-top", theme.levelsBackTop);
    field("levels-back-width", theme.levelsBackWidth);
    field("levels-more-width", theme.levelsMoreWidth);
    field("levels-card-left", theme.levelsCardLeft);
    field("levels-card-right", theme.levelsCardRight);
    field("levels-card-top", theme.levelsCardTop);
    field("levels-card-bottom", theme.levelsCardBottom);
    field("levels-card-color", theme.levelsCardColor);
    field("levels-card-frame-color", theme.levelsCardFrameColor);
    field("levels-card-shadow-color", theme.levelsCardShadowColor);
    field("levels-card-label-color", theme.levelsCardLabelColor);
    field("levels-card-text-color", theme.levelsCardTextColor);
    field("levels-card-accent-color", theme.levelsCardAccentColor);
    field("levels-card-hole-color", theme.levelsCardHoleColor);
    field("levels-play-text-color", theme.levelsPlayTextColor);
    field("levels-preview-empty-color", theme.levelsPreviewEmptyColor);
    field("levels-preview-fade", theme.levelsPreviewFade);
    field("levels-preview-width", theme.levelsPreviewWidth);

    field("achievements-enabled", theme.achievementsEnabled);
    field("achievements-title-left", theme.achievementsTitleLeft);
    field("achievements-title-cap-top", theme.achievementsTitleCapTop);
    field("achievements-counter-scale", theme.achievementsCounterScale);
    field("achievements-counter-color", theme.achievementsCounterColor);
    field("achievements-list-left", theme.achievementsListLeft);
    field("achievements-list-top", theme.achievementsListTop);
    field("achievements-list-bottom", theme.achievementsListBottom);
    field("achievements-row-width", theme.achievementsRowWidth);
    field("achievements-row-height", theme.achievementsRowHeight);
    field("achievements-row-gap", theme.achievementsRowGap);
    field("achievements-row-color", theme.achievementsRowColor);
    field("achievements-row-opacity", theme.achievementsRowOpacity);
    field("achievements-row-hover-color", theme.achievementsRowHoverColor);
    field("achievements-row-hover-opacity", theme.achievementsRowHoverOpacity);
    field("achievements-row-locked-color", theme.achievementsRowLockedColor);
    field("achievements-row-locked-opacity", theme.achievementsRowLockedOpacity);
    field("achievements-title-color", theme.achievementsTitleColor);
    field("achievements-title-locked-color", theme.achievementsTitleLockedColor);
    field("achievements-title-locked-opacity", theme.achievementsTitleLockedOpacity);
    field("achievements-title-scale", theme.achievementsTitleScale);
    field("achievements-text-color", theme.achievementsTextColor);
    field("achievements-text-scale", theme.achievementsTextScale);
    field("achievements-status-color", theme.achievementsStatusColor);
    field("achievements-icon-size", theme.achievementsIconSize);
    field("achievements-icon-frame-color", theme.achievementsIconFrameColor);
    field("achievements-icon-back-color", theme.achievementsIconBackColor);
    field("achievements-check-color", theme.achievementsCheckColor);
    field("achievements-progress-track-color", theme.achievementsProgressTrackColor);
    field("achievements-progress-fill-color", theme.achievementsProgressFillColor);
    field("achievements-scroll-left", theme.achievementsScrollLeft);
    field("achievements-scroll-width", theme.achievementsScrollWidth);
    field("achievements-scroll-track-color", theme.achievementsScrollTrackColor);
    field("achievements-scroll-thumb-color", theme.achievementsScrollThumbColor);
    field("achievements-back-top", theme.achievementsBackTop);
    field("achievements-back-width", theme.achievementsBackWidth);
    field("achievements-expand-duration", theme.achievementsExpandDuration);
    field("achievements-scroll-smoothing", theme.achievementsScrollSmoothing);
    field("pattern-sprite", theme.patternSprite);
    field("pattern-color", theme.patternColor);
    field("pattern-opacity", theme.patternOpacity);
    field("pattern-tile", theme.patternTile);
    field("pattern-depth", theme.patternDepth);
    field("vignette-sprite", theme.vignetteSprite);
    field("vignette-color", theme.vignetteColor);
    field("vignette-opacity", theme.vignetteOpacity);
    field("particle-count", theme.particleCount);
    field("particle-color", theme.particleColor);
    field("particle-opacity", theme.particleOpacity);
    field("particle-size-min", theme.particleSizeMin);
    field("particle-size-max", theme.particleSizeMax);
    field("particle-stretch", theme.particleStretch);
    field("particle-angle", theme.particleAngle);
    field("particle-turn", theme.particleTurn);
    field("particle-flock", theme.particleFlock);
    field("particle-flock-radius", theme.particleFlockRadius);
    field("particle-speed", theme.particleSpeed);
    field("particle-life", theme.particleLife);

    field("backdrop-middle-x", theme.backdropMiddleX);
    field("background-focus-x", theme.backgroundFocusX);
    field("background-focus-y", theme.backgroundFocusY);
    field("left-shade-width", theme.leftShadeWidth);
    field("glow-x", theme.glowX);
    field("glow-y", theme.glowY);
    field("glow-width", theme.glowWidth);
    field("glow-height", theme.glowHeight);
    field("character-x", theme.characterX);
    field("character-height", theme.characterHeight);
    field("character-depth", theme.characterDepth);
    field("parallax-range", theme.parallaxRange);

    field("mita-enabled", theme.mitaEnabled);
    field("mita-x", theme.mitaX);
    field("mita-y", theme.mitaY);
    field("mita-scale", theme.mitaScale);
    field("mita-fps", theme.mitaFps);
    field("mita-parallax-enabled", theme.mitaParallaxEnabled);
    field("mita-parallax-max-x", theme.mitaParallaxMaxX);
    field("mita-parallax-max-y", theme.mitaParallaxMaxY);
    field("mita-freeze-frame", theme.mitaFreezeFrame);
    field("mita-outline-enabled", theme.mitaOutlineEnabled);
    field("mita-outline-color", theme.mitaOutlineColor);
    field("mita-outline-opacity", theme.mitaOutlineOpacity);
    field("mita-outline-width", theme.mitaOutlineWidth);

    field("tray-margin", theme.trayMargin);
    field("tray-padding", theme.trayPadding);
    field("tray-gap", theme.trayGap);
    field("slot-size", theme.slotSize);
    field("slot-padding", theme.slotPadding);
    field("tray-max-width", theme.trayMaxWidth);
    field("card-left", theme.cardLeft);
    field("card-bottom", theme.cardBottom);
    field("card-padding", theme.cardPadding);
    field("card-gap", theme.cardGap);
    field("avatar-size", theme.avatarSize);
    field("avatar-icon", theme.avatarIcon);
    field("card-press-scale", theme.cardPressScale);
    field("popup-slot-size", theme.popupSlotSize);
    field("popup-columns", theme.popupColumns);
    field("popup-gap", theme.popupGap);
    field("popup-padding", theme.popupPadding);

    field("hover-duration", theme.hoverDuration);
    field("intro-duration", theme.introDuration);
    field("intro-stagger", theme.introStagger);
    field("intro-shift", theme.introShift);
    field("fade-duration", theme.fadeDuration);
    field("fade-back-duration", theme.fadeBackDuration);
    field("parallax-smoothing", theme.parallaxSmoothing);
  }

  std::optional<ccColor3B> parseColor(matjson::Value const& value)
  {
    auto color = value.asString().andThen([](std::string const& text) { return cc3bFromHexString(text); });
    if (!color)
      return std::nullopt;
    return color.unwrap();
  }

  matjson::Value toJson(Theme theme)
  {
    auto json = matjson::Value::object();
    fields(theme, [&](char const* key, auto& value) {
      using T = std::decay_t<decltype(value)>;
      if constexpr (std::is_same_v<T, ccColor3B>)
        json.set(key, cc3bToHexString(value));
      else if constexpr (std::is_same_v<T, std::vector<Theme::Layer>>)
      {
        auto layers = matjson::Value::array();
        for (auto const& layer : value)
        {
          auto entry = matjson::makeObject({{"sprite", layer.sprite},
                                            {"depth", layer.depth},
                                            {"blend", layer.additive ? "add" : "normal"},
                                            {"opacity", layer.opacity}});
          if (layer.tint)
            entry.set("tint", cc3bToHexString(*layer.tint));
          layers.push(entry);
        }
        json.set(key, layers);
      }
      else
        json.set(key, value);
    });
    return json;
  }

  // Fields that are missing or of the wrong type keep their defaults
  void apply(Theme& theme, matjson::Value const& json)
  {
    fields(theme, [&](char const* key, auto& value) {
      if (!json.contains(key))
        return;
      auto const& item = json[key];
      using T = std::decay_t<decltype(value)>;
      if constexpr (std::is_same_v<T, ccColor3B>)
      {
        if (auto color = parseColor(item))
          value = *color;
        else
          log::warn("theme.json: {} isn't a color like \"#ff6f91\"", key);
      }
      else if constexpr (std::is_same_v<T, std::vector<Theme::Layer>>)
      {
        if (!item.isArray())
        {
          log::warn("theme.json: {} isn't a list of layers", key);
          return;
        }
        value.clear();
        for (auto const& entry : item)
        {
          Theme::Layer layer;
          layer.sprite = entry["sprite"].asString().unwrapOr("");
          layer.depth = static_cast<float>(entry["depth"].asDouble().unwrapOr(0));
          layer.additive = entry["blend"].asString().unwrapOr("normal") == "add";
          layer.opacity =
            static_cast<GLubyte>(std::clamp<std::intmax_t>(entry["opacity"].asInt().unwrapOr(255), 0, 255));
          if (entry.contains("tint"))
            layer.tint = parseColor(entry["tint"]);
          value.push_back(layer);
        }
      }
      else if constexpr (std::is_same_v<T, std::string>)
        value = item.asString().unwrapOr(value);
      else if constexpr (std::is_same_v<T, bool>)
        value = item.asBool().unwrapOr(value);
      else if constexpr (std::is_same_v<T, GLubyte>)
        value = static_cast<GLubyte>(std::clamp<std::intmax_t>(item.asInt().unwrapOr(value), 0, 255));
      else
        value = static_cast<T>(item.asDouble().unwrapOr(value));
    });
  }
} // namespace

Theme const& Theme::get() { return s_theme; }

void Theme::reload()
{
  s_theme = defaults();
  auto dir = Mod::get()->getConfigDir();

  // Once a session: the list of every key with its default, to copy from
  static bool s_wroteDefaults = false;
  if (!s_wroteDefaults)
  {
    s_wroteDefaults = true;
    if (auto written = file::writeString(dir / "theme.defaults.json", toJson(s_theme).dump()); !written)
      log::warn("Can't write theme.defaults.json: {}", written.unwrapErr());
  }

  std::error_code error;
  auto path = dir / "theme.json";
  if (!std::filesystem::exists(path, error))
    return;
  auto json = file::readJson(path);
  if (!json)
  {
    log::warn("theme.json is ignored: {}", json.unwrapErr());
    return;
  }
  if (!json.unwrap().isObject())
  {
    log::warn("theme.json is ignored: it isn't an object");
    return;
  }
  apply(s_theme, json.unwrap());
  log::debug("Theme overrides read from {}", path);
}

ccColor4B Theme::withAlpha(ccColor3B color, GLubyte opacity) { return ccc4(color.r, color.g, color.b, opacity); }
