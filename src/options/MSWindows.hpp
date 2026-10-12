#pragma once

#include <Geode/Geode.hpp>

// The windows GD's settings open, as screens of the mod's: the account (and its More), the soundtracks with each
// song's window, how to play, the support window and the lists of links it opens. Each works GD's own window, made for
// it and never shown: its buttons are pressed, its texts shown, and the screen follows the window as it changes (signed
// in, saving...). GD WINDOW on each opens GD's own
namespace ms::options
{
  // GD's window that drops down, over everything, as GD's settings add it
  void dropDown(GJDropDownLayer* layer);

  void openAccount();
  void openSoundtracks();
  void openSongInfo(int songID);
  void openTutorial();
  void openSupport();
  // GD's list of links (the support's links and sound credits) as the mod's screen
  void openLinks(URLViewLayer* links);
} // namespace ms::options
