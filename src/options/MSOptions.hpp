#pragma once

#include <Geode/Geode.hpp>

// GD's settings as screens of MiSide's, in place of its own windows: the volumes and the menu music, every page of
// GD's options by its group (with the music offset, the saved songs, FMOD's debug, the parental control and the key
// bindings where GD has them), the graphics with their advanced options, and GD's account, soundtracks, help, rate and
// vault. Every row works GD's own window, made and never shown: a toggle is a click on GD's own button for it, so
// everything GD does on that click happens. GD WINDOW opens the window as GD has it (with other mods' buttons)
namespace ms::options
{
  // vanilla: opens GD's own window once the screen has gone (GD puts it in the menu, which hides under the screen);
  // graphics: the graphics' screen over it too, as GD opens its video options again after applying them
  void open(std::function<void()> vanilla, bool graphics = false);
} // namespace ms::options
