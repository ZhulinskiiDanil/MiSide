#include "Theme.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

class $modify(MSMenuLayer, MenuLayer)
{
  bool init()
  {
    if (!MenuLayer::init())
      return false;
    // The vanilla menu: nothing else happens here
    if (Mod::get()->getSettingValue<bool>("vanilla-menu"))
      return true;

    Theme::reload();
    return true;
  }
};
