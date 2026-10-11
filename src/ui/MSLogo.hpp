#pragma once

#include <Geode/Geode.hpp>

#include <string>

// The logo over the menu: the theme's logo image as it is, nothing drawn over it. Its transparent margins are left
// out: the node is the image's drawn part only, so its bottom left corner is the drawing's
class MSLogo : public cocos2d::CCNode
{
public:
  // Null when the image is missing (logged once)
  static MSLogo* create();

  // Scaled so its drawn part is `logo-width` points wide on the screen, under parents scaled this much
  void fit(float parentScale);

protected:
  bool init() override;
  // The drawn part's bounds in the image's pixels (alpha above a faint threshold), from the file itself
  bool measure(std::string const& path);

  cocos2d::CCSprite* m_sprite = nullptr;
  // In the image's pixels, from its top left corner
  cocos2d::CCRect m_drawn;
  cocos2d::CCSize m_pixels;
};
