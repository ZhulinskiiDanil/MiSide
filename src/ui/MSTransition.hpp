#pragma once

#include <Geode/Geode.hpp>

#include <functional>

// A short darkening over the menu before an item's vanilla button runs. It lives in the menu layer alone:
// no transition is hooked, so every other screen changes scenes exactly as before
class MSTransition : public cocos2d::CCLayerColor
{
public:
  static MSTransition* create();

  // Darkens, then runs the action. When the action started a scene change, the darkening stays and the
  // vanilla transition takes the scene from there; otherwise (a popup opened) it fades back and done() runs
  void play(std::function<void()> action, std::function<void()> done);

protected:
  bool init() override;
};
