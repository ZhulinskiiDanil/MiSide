#pragma once

// The appearance settings: a screen of the mod's over the menu with the mod's own settings (its language, logo,
// intro, the level exit's confirmation, the parallax, the vanilla menu) and GD's music and effects volumes, grouped,
// with their values; the mod's settings back to their defaults with one row. They're the same settings Geode keeps
// (no copy of them), so its own settings page shows the same values
namespace ms::settings
{
  void open();
} // namespace ms::settings
