#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

#include <string>
#include <vector>

// The buttons that aren't in the tray's row: RobTop, the social links, More Games, and whatever mods add to their
// menus. Each one shows its vanilla button's sprite and presses that button with activate()
class MSMorePopup : public geode::Popup
{
public:
  static MSMorePopup* create(std::vector<cocos2d::CCMenuItem*> targets, std::string const& title);

protected:
  bool init(std::vector<cocos2d::CCMenuItem*> const& targets, std::string const& title);
};
