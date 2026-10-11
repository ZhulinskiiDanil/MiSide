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
    theme.itemColor = ccc3(0xe8, 0xdc, 0xe6);
    theme.itemHoverColor = ccc3(0xff, 0xff, 0xff);
    theme.itemPressedColor = ccc3(0xff, 0x9f, 0xb8);
    theme.accentColor = ccc3(0xff, 0x6f, 0x91);
    theme.fadeColor = ccc3(0x0b, 0x06, 0x0d);
    theme.trayColor = ccc3(0x1a, 0x10, 0x20);
    theme.slotColor = ccc3(0xff, 0xff, 0xff);
    theme.itemOpacity = 200;
    theme.itemHoverOpacity = 255;
    theme.fadeOpacity = 200;
    theme.trayOpacity = 170;
    theme.slotOpacity = 28;

    // ! --- Fonts --- !
    theme.font = "miside-font.fnt"_spr;
    theme.fallbackFont = "Arial";
    theme.fallbackFontSize = 24;
    theme.itemScale = 1.1f;

    // ! --- Sprites and sounds --- !
    theme.layers = {
      {"bg-far.png"_spr, 0.2f},
      {"bg-mid.png"_spr, 0.5f},
      {"bg-near.png"_spr, 1.f},
      {"bg-vignette.png"_spr, 0.f},
    };
    theme.accentSprite = "item-accent.png"_spr;
    theme.traySprite = "tray-panel.png"_spr;
    theme.slotSprite = "tray-slot.png"_spr;
    theme.hoverSound = "hover.ogg"_spr;
    theme.clickSound = "click.ogg"_spr;
    theme.soundVolume = 0.5f;

    // ! --- Spacing --- !
    theme.columnLeft = 42;
    theme.columnTop = 0.72f;
    theme.itemGap = 6;
    theme.accentWidth = 3;
    theme.accentGap = 8;
    theme.hoverShift = 10;
    theme.pressedScale = 0.96f;
    theme.trayMargin = 10;
    theme.trayPadding = 6;
    theme.trayRowHeight = 30;
    theme.trayGap = 6;
    theme.trayMaxWidth = 0.55f;
    theme.trayMaxHeight = 0.6f;
    theme.slotPadding = 3;
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
    field("accent-color", theme.accentColor);
    field("fade-color", theme.fadeColor);
    field("tray-color", theme.trayColor);
    field("slot-color", theme.slotColor);
    field("item-opacity", theme.itemOpacity);
    field("item-hover-opacity", theme.itemHoverOpacity);
    field("fade-opacity", theme.fadeOpacity);
    field("tray-opacity", theme.trayOpacity);
    field("slot-opacity", theme.slotOpacity);

    field("font", theme.font);
    field("fallback-font", theme.fallbackFont);
    field("fallback-font-size", theme.fallbackFontSize);
    field("item-scale", theme.itemScale);

    field("layers", theme.layers);
    field("accent-sprite", theme.accentSprite);
    field("tray-sprite", theme.traySprite);
    field("slot-sprite", theme.slotSprite);
    field("hover-sound", theme.hoverSound);
    field("click-sound", theme.clickSound);
    field("sound-volume", theme.soundVolume);

    field("column-left", theme.columnLeft);
    field("column-top", theme.columnTop);
    field("item-gap", theme.itemGap);
    field("accent-width", theme.accentWidth);
    field("accent-gap", theme.accentGap);
    field("hover-shift", theme.hoverShift);
    field("pressed-scale", theme.pressedScale);
    field("tray-margin", theme.trayMargin);
    field("tray-padding", theme.trayPadding);
    field("tray-row-height", theme.trayRowHeight);
    field("tray-gap", theme.trayGap);
    field("tray-max-width", theme.trayMaxWidth);
    field("tray-max-height", theme.trayMaxHeight);
    field("slot-padding", theme.slotPadding);
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
          layers.push(matjson::makeObject({{"sprite", layer.sprite}, {"depth", layer.depth}}));
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
                           static_cast<float>(entry["depth"].asDouble().unwrapOr(0))});
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
