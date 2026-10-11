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
    theme.backdropTop = ccc3(0x2a, 0x18, 0x30);
    theme.backdropBottom = ccc3(0x12, 0x0a, 0x16);
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
    theme.glowColor = ccc3(0xb0, 0x12, 0x7a);
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
    theme.glowOpacity = 100;
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
    theme.layers = {
      {"bg-far.png"_spr, 0.2f, false, 255},
      {"bg-mid.png"_spr, 0.5f, false, 220},
      {"bg-near.png"_spr, 1.f, false, 110},
      {"bg-light.png"_spr, 0.f, true, 35},
    };
    theme.rowSprite = "row-plate.png"_spr;
    theme.logoSprite = "miside-like-gd-logo.png"_spr;
    theme.glowSprite = "glow-soft.png"_spr;
    theme.characterSprite = "";
    theme.accentSprite = "item-accent.png"_spr;
    theme.traySprite = "tray-panel.png"_spr;
    theme.slotSprite = "tray-slot.png"_spr;
    theme.avatarSprite = "avatar-circle.png"_spr;
    theme.hoverSound = "hover.ogg"_spr;
    theme.clickSound = "click.ogg"_spr;
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
    theme.pauseAmbientEnabled = false;
    theme.pauseAmbientPath = "";
    theme.pauseAmbientVolume = 0.5f;
    theme.pauseAmbientFade = 1;
    theme.pauseExtraEnabled = true;
    theme.pauseExtraMenus = "left-button-menu,right-button-menu,bottom-button-menu";
    theme.pauseDuration = 0.2f;

    // ! --- Decoration over the room --- !
    theme.patternSprite = "pattern-tile.png"_spr;
    theme.patternColor = ccc3(0xff, 0xb8, 0xdc);
    theme.patternOpacity = 16;
    theme.patternTile = 64;
    theme.vignetteSprite = "vignette-soft.png"_spr;
    theme.vignetteColor = ccc3(0x12, 0x03, 0x0e);
    theme.vignetteOpacity = 180;
    theme.particleCount = 14;
    theme.particleColor = ccc3(0xff, 0x8c, 0xc6);
    theme.particleOpacity = 150;
    theme.particleSizeMin = 1.2f;
    theme.particleSizeMax = 3.f;
    theme.particleSpeed = 5;
    theme.particleLife = 8;

    // ! --- The background --- !
    theme.backgroundFocusX = 0.5f;
    theme.backgroundFocusY = 0.5f;
    theme.leftShadeWidth = 260;
    theme.glowX = 0.84f;
    theme.glowY = 0.55f;
    theme.glowWidth = 340;
    theme.glowHeight = 300;
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
    field("backdrop-top", theme.backdropTop);
    field("backdrop-bottom", theme.backdropBottom);
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

    field("pattern-sprite", theme.patternSprite);
    field("pattern-color", theme.patternColor);
    field("pattern-opacity", theme.patternOpacity);
    field("pattern-tile", theme.patternTile);
    field("vignette-sprite", theme.vignetteSprite);
    field("vignette-color", theme.vignetteColor);
    field("vignette-opacity", theme.vignetteOpacity);
    field("particle-count", theme.particleCount);
    field("particle-color", theme.particleColor);
    field("particle-opacity", theme.particleOpacity);
    field("particle-size-min", theme.particleSizeMin);
    field("particle-size-max", theme.particleSizeMax);
    field("particle-speed", theme.particleSpeed);
    field("particle-life", theme.particleLife);

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
