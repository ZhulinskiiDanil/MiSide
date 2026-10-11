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
    theme.itemColor = ccc3(0xf6, 0xf0, 0xf4);
    theme.itemHoverColor = ccc3(0xff, 0xff, 0xff);
    theme.itemPressedColor = ccc3(0xff, 0xb3, 0xc7);
    theme.titleColor = ccc3(0xff, 0xff, 0xff);
    theme.shadowColor = ccc3(0x12, 0x07, 0x14);
    theme.accentColor = ccc3(0xff, 0x6f, 0x91);
    theme.fadeColor = ccc3(0x0b, 0x06, 0x0d);
    theme.trayColor = ccc3(0x1a, 0x10, 0x20);
    theme.slotColor = ccc3(0xff, 0xff, 0xff);
    theme.cardColor = ccc3(0x1a, 0x10, 0x20);
    theme.avatarColor = ccc3(0x2e, 0x1b, 0x34);
    theme.nameColor = ccc3(0xf6, 0xf0, 0xf4);
    theme.popupColor = ccc3(0x22, 0x14, 0x28);
    theme.itemOpacity = 225;
    theme.itemHoverOpacity = 255;
    theme.shadowOpacity = 130;
    theme.fadeOpacity = 200;
    theme.trayOpacity = 150;
    theme.slotOpacity = 28;
    theme.cardOpacity = 150;
    theme.avatarOpacity = 230;
    theme.popupOpacity = 240;

    // ! --- Fonts --- !
    theme.font = "miside-font.fnt"_spr;
    theme.fallbackFont = "Arial";
    theme.fallbackFontSize = 24;
    theme.itemScale = 1.1f;
    theme.titleScale = 1.45f;
    theme.nameScale = 0.7f;
    theme.moreScale = 0.9f;
    theme.popupTitleScale = 1.f;
    theme.shadowSteps = 2;
    theme.shadowOffset = 1.f;

    // ! --- Sprites and sounds --- !
    theme.layers = {
      {"bg-far.png"_spr, 0.2f},
      {"bg-mid.png"_spr, 0.5f},
      {"bg-near.png"_spr, 1.f},
      // About a third fainter than the file
      {"bg-light.png"_spr, 0.f, true, 170},
    };
    theme.accentSprite = "item-accent.png"_spr;
    theme.traySprite = "tray-panel.png"_spr;
    theme.slotSprite = "tray-slot.png"_spr;
    theme.avatarSprite = "avatar-circle.png"_spr;
    theme.hoverSound = "hover.ogg"_spr;
    theme.clickSound = "click.ogg"_spr;
    theme.soundVolume = 0.5f;

    // ! --- Spacing --- !
    theme.columnLeft = 42;
    theme.columnTop = 0.86f;
    theme.columnClearance = 8;
    theme.titleGap = 12;
    theme.itemGap = 6;
    theme.accentWidth = 3;
    theme.accentGap = 8;
    theme.hoverShift = 10;
    theme.pressedScale = 0.96f;
    theme.trayMargin = 10;
    theme.trayPadding = 4;
    theme.trayGap = 3;
    // About 65% of the first tray's slots
    theme.slotSize = 17;
    theme.slotPadding = 2;
    theme.trayMaxWidth = 0.6f;
    theme.cardPadding = 4;
    theme.cardGap = 6;
    theme.avatarSize = 24;
    theme.avatarIcon = 0.7f;
    theme.cardPressScale = 1.06f;
    theme.popupSlotSize = 30;
    theme.popupColumns = 4;
    theme.popupGap = 8;
    theme.popupPadding = 14;
    theme.parallaxRange = 14;

    // ! --- Timings --- !
    theme.hoverDuration = 0.12f;
    theme.introDuration = 0.35f;
    theme.introStagger = 0.05f;
    theme.introShift = 18;
    theme.fadeDuration = 0.18f;
    theme.fadeBackDuration = 0.2f;
    theme.parallaxSmoothing = 0.12f;
    return theme;
  }

  // Every field with its key in theme.json
  template <class F>
  void fields(Theme& theme, F&& field)
  {
    field("backdrop-top", theme.backdropTop);
    field("backdrop-bottom", theme.backdropBottom);
    field("item-color", theme.itemColor);
    field("item-hover-color", theme.itemHoverColor);
    field("item-pressed-color", theme.itemPressedColor);
    field("title-color", theme.titleColor);
    field("shadow-color", theme.shadowColor);
    field("accent-color", theme.accentColor);
    field("fade-color", theme.fadeColor);
    field("tray-color", theme.trayColor);
    field("slot-color", theme.slotColor);
    field("card-color", theme.cardColor);
    field("avatar-color", theme.avatarColor);
    field("name-color", theme.nameColor);
    field("popup-color", theme.popupColor);
    field("item-opacity", theme.itemOpacity);
    field("item-hover-opacity", theme.itemHoverOpacity);
    field("shadow-opacity", theme.shadowOpacity);
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
    field("title-scale", theme.titleScale);
    field("name-scale", theme.nameScale);
    field("more-scale", theme.moreScale);
    field("popup-title-scale", theme.popupTitleScale);
    field("shadow-steps", theme.shadowSteps);
    field("shadow-offset", theme.shadowOffset);

    field("layers", theme.layers);
    field("accent-sprite", theme.accentSprite);
    field("tray-sprite", theme.traySprite);
    field("slot-sprite", theme.slotSprite);
    field("avatar-sprite", theme.avatarSprite);
    field("hover-sound", theme.hoverSound);
    field("click-sound", theme.clickSound);
    field("sound-volume", theme.soundVolume);

    field("column-left", theme.columnLeft);
    field("column-top", theme.columnTop);
    field("column-clearance", theme.columnClearance);
    field("title-gap", theme.titleGap);
    field("item-gap", theme.itemGap);
    field("accent-width", theme.accentWidth);
    field("accent-gap", theme.accentGap);
    field("hover-shift", theme.hoverShift);
    field("pressed-scale", theme.pressedScale);
    field("tray-margin", theme.trayMargin);
    field("tray-padding", theme.trayPadding);
    field("tray-gap", theme.trayGap);
    field("slot-size", theme.slotSize);
    field("slot-padding", theme.slotPadding);
    field("tray-max-width", theme.trayMaxWidth);
    field("card-padding", theme.cardPadding);
    field("card-gap", theme.cardGap);
    field("avatar-size", theme.avatarSize);
    field("avatar-icon", theme.avatarIcon);
    field("card-press-scale", theme.cardPressScale);
    field("popup-slot-size", theme.popupSlotSize);
    field("popup-columns", theme.popupColumns);
    field("popup-gap", theme.popupGap);
    field("popup-padding", theme.popupPadding);
    field("parallax-range", theme.parallaxRange);

    field("hover-duration", theme.hoverDuration);
    field("intro-duration", theme.introDuration);
    field("intro-stagger", theme.introStagger);
    field("intro-shift", theme.introShift);
    field("fade-duration", theme.fadeDuration);
    field("fade-back-duration", theme.fadeBackDuration);
    field("parallax-smoothing", theme.parallaxSmoothing);
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
          layers.push(matjson::makeObject(
            {{"sprite", layer.sprite},
             {"depth", layer.depth},
             {"blend", layer.additive ? "add" : "normal"},
             {"opacity", layer.opacity}}));
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
        auto color = item.asString().andThen([](std::string const& text) { return cc3bFromHexString(text); });
        if (color)
          value = color.unwrap();
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
          value.push_back({entry["sprite"].asString().unwrapOr(""),
                           static_cast<float>(entry["depth"].asDouble().unwrapOr(0)),
                           entry["blend"].asString().unwrapOr("normal") == "add",
                           static_cast<GLubyte>(std::clamp<std::intmax_t>(entry["opacity"].asInt().unwrapOr(255), 0, 255))});
      }
      else if constexpr (std::is_same_v<T, std::string>)
        value = item.asString().unwrapOr(value);
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
  apply(s_theme, json.unwrap());
  log::debug("Theme overrides read from {}", path);
}

ccColor4B Theme::withAlpha(ccColor3B color, GLubyte opacity) { return ccc4(color.r, color.g, color.b, opacity); }
