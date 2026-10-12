#include "MSButtonMenu.hpp"

#include "../Theme.hpp"
#include "MSStyle.hpp"

using namespace geode::prelude;

MSButtonMenu* MSButtonMenu::create()
{
  auto menu = new MSButtonMenu();
  if (menu->init())
  {
    menu->autorelease();
    return menu;
  }
  delete menu;
  return nullptr;
}

bool MSButtonMenu::init()
{
  if (!CCMenu::init())
    return false;
  this->setPosition({0, 0});
  this->setContentSize({0, 0});
  this->scheduleUpdate();
  return true;
}

void MSButtonMenu::resetLook()
{
  for (auto item : CCArrayExt<CCNode*>(this->getChildren()))
  {
    if (auto row = typeinfo_cast<MSMenuItem*>(item))
      row->setHighlighted(false);
  }
}

void MSButtonMenu::update(float)
{
  auto mouse = getMousePos();
  if (mouse.equals(m_lastMouse))
    return;
  m_lastMouse = mouse;
  // Hidden (its layer hidden by GD's hide button) or covered: nothing lights
  auto usable = !ms::coveredBy(this);
  for (CCNode* node = this; node && usable; node = node->getParent())
    usable = node->isVisible();
  auto local = this->convertToNodeSpace(mouse);
  for (auto child : CCArrayExt<CCNode*>(this->getChildren()))
  {
    auto row = typeinfo_cast<MSMenuItem*>(child);
    if (!row)
      continue;
    auto over = usable && row->isVisible() && row->isEnabled() && row->boundingBox().containsPoint(local);
    if (over == row->isHighlighted())
      continue;
    row->setHighlighted(over, true);
    if (over)
      ms::playSound(Theme::get().hoverSound);
  }
}
