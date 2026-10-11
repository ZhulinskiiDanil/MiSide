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
    theme.rowColor = ccc3(0x76, 0x24, 0x75);
    theme.rowHoverColor = ccc3(0xba, 0x16, 0x8e);
    theme.rowPressedColor = ccc3(0xd5, 0x2a, 0x9d);
    theme.rowDisabledColor = ccc3(0x35, 0x10, 0x2e);
    theme.itemColor = ccc3(0xf9, 0xd8, 0xf2);
    theme.itemHoverColor = ccc3(0xff, 0xf0, 0xfb);
    theme.itemPressedColor = ccc3(0xff, 0xff, 0xff);
    theme.subtitleColor = ccc3(0xff, 0xf5, 0xfc);
    theme.shadowColor = ccc3(0x21, 0x09, 0x1e);
    theme.accentColor = ccc3(0xff, 0x6f, 0x91);
    theme.logoColor = ccc3(0xff, 0x3d, 0x9e);
    theme.logoHighlightColor = ccc3(0xff, 0xc6, 0xe6);
    theme.logoShadowColor = ccc3(0x3b, 0x0a, 0x2f);
    theme.logoDecorColor = ccc3(0xff, 0x8a, 0xc8);
    theme.backgroundTint = ccc3(0x8c, 0x52, 0x7e);
    theme.darkenColor = ccc3(0x15, 0x05, 0x1a);
    theme.leftShadeColor = ccc3(0x08, 0x05, 0x0d);
    theme.glowColor = ccc3(0x9e, 0x08, 0x6f);
    theme.fadeColor = ccc3(0x09, 0x04, 0x0d);
    theme.trayColor = ccc3(0x1a, 0x10, 0x20);
    theme.slotColor = ccc3(0xff, 0xff, 0xff);
    theme.cardColor = ccc3(0x16, 0x0a, 0x1d);
    theme.avatarColor = ccc3(0x2e, 0x1b, 0x34);
    theme.nameColor = ccc3(0xf9, 0xd8, 0xf2);
    theme.popupColor = ccc3(0x22, 0x14, 0x28);
    theme.rowOpacity = 190;
    theme.rowHoverOpacity = 235;
    theme.rowPressedOpacity = 245;
    theme.rowDisabledOpacity = 100;
    theme.itemOpacity = 255;
    theme.itemHoverOpacity = 255;
    theme.shadowOpacity = 100;
    theme.darkenOpacity = 125;
    theme.leftShadeOpacity = 190;
    theme.glowOpacity = 90;
    theme.characterOpacity = 255;
    theme.fadeOpacity = 200;
    theme.trayOpacity = 80;
    theme.slotOpacity = 0;
    theme.cardOpacity = 90;
    theme.avatarOpacity = 230;
    theme.popupOpacity = 240;

    // ! --- Fonts --- !
    theme.font = "miside-font.fnt"_spr;
    theme.fallbackFont = "Arial";
    theme.fallbackFontSize = 24;
    // Capitals about 8.5 pt high: the font's are 11.3 pt at scale 1
    theme.itemScale = 0.75f;
    theme.itemTextOffset = 0.6f;
    // Capitals about 10 pt high
    theme.subtitleScale = 0.9f;
    theme.nameScale = 0.5f;
    theme.moreScale = 0.9f;
    theme.popupTitleScale = 0.9f;
    theme.shadowSteps = 1;
    theme.shadowOffset = 0.5f;
    theme.logoShadowOffset = 1.2f;
    theme.logoHighlightOffset = 0.7f;
    theme.logoDecorSize = 0.4f;

    // ! --- Sprites and sounds --- !
    theme.layers = {
      {"bg-far.png"_spr, 0.2f, false, 255},
      {"bg-mid.png"_spr, 0.5f, false, 220},
      {"bg-near.png"_spr, 1.f, false, 110},
      {"bg-light.png"_spr, 0.f, true, 35},
    };
    theme.rowSprite = "row-plate.png"_spr;
    theme.logoSprite = "";
    theme.glowSprite = "glow-soft.png"_spr;
    theme.characterSprite = "";
    theme.accentSprite = "item-accent.png"_spr;
    theme.traySprite = "tray-panel.png"_spr;
    theme.slotSprite = "tray-slot.png"_spr;
    theme.avatarSprite = "avatar-circle.png"_spr;
    theme.hoverSound = "hover.ogg"_spr;
    theme.clickSound = "click.ogg"_spr;
    theme.soundVolume = 0.5f;

    // ! --- The menu --- !
    theme.columnLeft = 59;
    theme.logoBottom = 244;
    theme.logoTop = 280;
    theme.subtitleTop = 233;
    theme.firstRowY = 195;
    theme.rowPitch = 16;
    theme.rowWidth = 148;
    theme.rowHeight = 14;
    theme.textPadding = 3;
    theme.exitWidth = 59;
    theme.exitY = 37;
    theme.columnClearance = 6;
    theme.accentEnabled = false;
    theme.accentWidth = 0;
    theme.accentGap = 0;
    theme.hoverShift = 0;
    theme.pressedScale = 1.f;

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
    field("logo-color", theme.logoColor);
    field("logo-highlight-color", theme.logoHighlightColor);
    field("logo-shadow-color", theme.logoShadowColor);
    field("logo-decor-color", theme.logoDecorColor);
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
    field("logo-shadow-offset", theme.logoShadowOffset);
    field("logo-highlight-offset", theme.logoHighlightOffset);
    field("logo-decor-size", theme.logoDecorSize);

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

    field("column-left", theme.columnLeft);
    field("logo-bottom", theme.logoBottom);
    field("logo-top", theme.logoTop);
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
