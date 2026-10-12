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
  // The custom songs downloaded on this device, each with its own buttons
  void openSavedSongs();
  // GD's list of links (the support's links and sound credits) as the mod's screen
  void openLinks(URLViewLayer* links);
  // GD's popups that stay at work while shown (a request's progress, the UI's save slots): the mod's screen over them,
  // following them
  void mirrorPopup(FLAlertLayer* popup);
  // GD's reward chest with the mod's column over it
  void rewardOverlay(RewardUnlockLayer* reward);
  // A level's own options (its page's options button)
  void openLevelOptions(GJGameLevel* level);
} // namespace ms::options
