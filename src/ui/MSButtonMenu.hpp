#pragma once

#include "MSMenuItem.hpp"

#include <Geode/Geode.hpp>

// A menu of the mod's items outside a column (a card's buttons): the item under the mouse lights up with its shine and
// the hover sound, as the column's rows do; a popup over it stops that
class MSButtonMenu : public cocos2d::CCMenu
{
public:
  static MSButtonMenu* create();

  void update(float dt) override;
  // Nothing lit (the screen changed)
  void resetLook();

protected:
  bool init() override;

  cocos2d::CCPoint m_lastMouse;
};
