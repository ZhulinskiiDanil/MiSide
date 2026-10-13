#pragma once

#include <Geode/Geode.hpp>

#include <vector>

// Going from one page of the menu to another, as MiSide does: the page going slides right and fades out, step after
// step from the top; the page coming slides in from the left into its places the same way, a little later, so the
// two overlap in a staircase. A step's nodes move together (a row's plate and text, a slider's track). Fast first,
// slowing down to rest, no spring. Times and distances are the theme's `page-*`
namespace ms::wave
{
  struct Step
  {
    std::vector<cocos2d::CCNode*> nodes;
    // Off: it only fades (a scroll bar, the tray)
    bool slide = true;
  };
  using Steps = std::vector<Step>;

  // Shown, or hidden by a wave that took it out (the next wave in brings it back)
  bool shows(cocos2d::CCNode* node);
  // A wave is moving it now
  bool moving(cocos2d::CCNode* node);

  // The steps' shown nodes go; once out they're hidden, back in place and as opaque as they were. Seconds until the
  // last one is out
  float out(Steps const& steps, float delay = 0);
  // The steps' nodes come in: the shown ones, and the ones a wave took out. Seconds until the last one is in
  float in(Steps const& steps, float delay = 0);
  // At once, as a wave would end: the shown nodes hidden (a wave in brings them back), or those back
  void hide(Steps const& steps);
  void show(Steps const& steps);
} // namespace ms::wave
