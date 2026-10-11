#pragma once

#include <Geode/Geode.hpp>

#include <string>

// The player's card in the bottom left corner: a round avatar with their icon and their name. It presses the
// vanilla profile button with activate(), so the mods hooking it keep working
class MSProfileCard : public cocos2d::CCMenu
{
public:
  static MSProfileCard* create(cocos2d::CCMenuItem* profileButton, std::string const& name);

  // What the card takes on the screen, in its parent's space
  cocos2d::CCRect getArea();

protected:
  bool init(cocos2d::CCMenuItem* profileButton, std::string const& name);
  cocos2d::CCNode* createAvatar();

  cocos2d::CCMenuItem* m_button = nullptr;
};
