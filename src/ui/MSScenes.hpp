#pragma once

#include <Geode/Geode.hpp>

#include <functional>

// Going between two scenes of the mod's that look alike (the main menu, CREATE): the page going waves out, then the
// next scene comes in at once instead of through GD's fade through black, and its page waves in. Its background,
// logo and Mita are where the last one's were, so it looks like one menu
namespace ms::scenes
{
  // The scene GD changes to next, this frame or the next, comes in without its transition
  void skipNextTransition();
  // Whether a scene change took the skip since it was armed (false: nothing changed scene, the page comes back)
  bool skipped();
  // Armed and not taken yet: a scene being made now is coming in at once (Mita skips her fade-in)
  bool comingAtOnce();
  // `then` once `node`'s scene is the running one, its transition over (at once if it already is)
  void whenShown(cocos2d::CCNode* node, std::function<void()> then);
} // namespace ms::scenes
