#pragma once

#include <Geode/Geode.hpp>

// The logo over the menu, fitted into the theme's logo area: the theme's logo art when there is one, otherwise a
// text logo (a stand-in until the art is made, not the art itself). Anchored at its bottom left corner
class MSLogo : public cocos2d::CCNode
{
public:
  static MSLogo* create();

protected:
  bool init() override;
  // "Geometry Dash" in pink with a light edge on top, a dark shadow under it, and a GD cube and spike after it
  cocos2d::CCNode* createWordmark();
};
