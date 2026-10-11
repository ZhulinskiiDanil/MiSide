#pragma once

#include <Geode/Geode.hpp>

// The logo over the menu, fitted into the theme's logo area: the theme's logo art when there is one, otherwise a
// text logo built from the title font (a stand-in for real art). Anchored at its bottom left corner
class MSLogo : public cocos2d::CCNode
{
public:
  static MSLogo* create();

protected:
  bool init() override;
  // "Geometry Dash": a pink gradient cut to the letters, a few darker copies under them for depth, a soft shadow,
  // sparkles on the letters' tops
  cocos2d::CCNode* createWordmark();
  // Thin lines behind the logo
  cocos2d::CCNode* createGrid(cocos2d::CCSize area);
};
