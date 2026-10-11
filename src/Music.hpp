#pragma once

#include <string>

// The mod's music: which files GD plays for the main menu and the icon kit, and how loud each is
namespace music
{
  // A file name in the mod's config folder or resources, or a path: its full path, or empty when it isn't there
  std::string find(std::string const& name);
  // The main menu's track (empty: GD's own)
  std::string menuTrack();
  // The icon kit's track (empty: none, the menu's goes on)
  std::string garageTrack();
} // namespace music
