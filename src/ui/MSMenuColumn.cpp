#include "MSMenuColumn.hpp"

#include "../Theme.hpp"
#include "MSStyle.hpp"

using namespace geode::prelude;

MSMenuColumn* MSMenuColumn::create(Choose choose)
{
  auto column = new MSMenuColumn();
  if (column->init(std::move(choose)))
  {
    column->autorelease();
    return column;
  }
  delete column;
  return nullptr;
}

bool MSMenuColumn::init(Choose choose)
{
  if (!CCMenu::init())
    return false;
  m_choose = std::move(choose);
  // The items hang down from the column's origin, its top left corner; fit() places and scales it
  this->setContentSize({0, 0});
  m_lastMouse = getMousePos();
  this->scheduleUpdate();
  return true;
}

void MSMenuColumn::addEntry(std::string const& text, CCMenuItem* target)
{
  auto item = MSMenuItem::create(text, target, [this](MSMenuItem* item) { this->choose(item); });
  if (!item)
    return;
  m_items.push_back(item);
  this->addChild(item);
}

void MSMenuColumn::finish()
{
  auto const& theme = Theme::get();
  auto y = 0.f;
  m_width = 0;
  for (size_t i = 0; i < m_items.size(); i++)
  {
    auto item = m_items[i];
    auto height = item->getContentHeight();
    item->setPosition({0, -(y + height / 2)});
    y += height + theme.itemGap;
    m_width = std::max(m_width, item->getContentWidth());
    item->playIntro(theme.introStagger * i);
  }
  m_height = m_items.empty() ? 0 : y - theme.itemGap;
  auto winSize = CCDirector::get()->getWinSize();
  this->fit(winSize.height * theme.columnTop, 0);
}

void MSMenuColumn::fit(float top, float bottom)
{
  auto scale = 1.f;
  if (m_height > 0 && top - bottom < m_height)
    scale = std::max(top - bottom, 0.f) / m_height;
  this->setScale(scale);
  this->setPosition({Theme::get().columnLeft, top});
}

CCRect MSMenuColumn::getArea()
{
  auto scale = this->getScale();
  auto position = this->getPosition();
  return {position.x, position.y - m_height * scale, m_width * scale, m_height * scale};
}

void MSMenuColumn::setBusy(bool busy) { m_busy = busy; }

bool MSMenuColumn::isOnTop()
{
  auto layer = this->getParent();
  auto scene = layer ? layer->getParent() : nullptr;
  if (!scene || CCDirector::get()->getRunningScene() != scene || CCDirector::get()->getNextScene())
    return false;
  // Popups are layers added to the scene after the menu (Geode's overlays aren't in the scene)
  auto above = false;
  for (auto child : CCArrayExt<CCNode*>(scene->getChildren()))
  {
    if (child == layer)
    {
      above = true;
      continue;
    }
    if (!above || !child->isVisible() || !typeinfo_cast<CCLayer*>(child))
      continue;
    return false;
  }
  return true;
}

void MSMenuColumn::update(float)
{
  auto mouse = getMousePos();
  if (mouse.equals(m_lastMouse))
    return;
  m_lastMouse = mouse;
  if (m_busy || !this->isOnTop())
    return;

  auto local = this->convertToNodeSpace(mouse);
  auto under = -1;
  for (size_t i = 0; i < m_items.size(); i++)
  {
    if (m_items[i]->boundingBox().containsPoint(local))
    {
      under = static_cast<int>(i);
      break;
    }
  }
  this->highlight(under, true);
}

void MSMenuColumn::highlight(int index, bool sound)
{
  if (index == m_current)
    return;
  if (m_current >= 0 && m_current < static_cast<int>(m_items.size()))
    m_items[m_current]->setHighlighted(false);
  m_current = index;
  if (m_current < 0 || m_current >= static_cast<int>(m_items.size()))
  {
    m_current = -1;
    return;
  }
  m_items[m_current]->setHighlighted(true);
  if (sound)
    ms::playSound(Theme::get().hoverSound);
}

bool MSMenuColumn::onKey(enumKeyCodes key)
{
  if (m_items.empty() || m_busy || !this->isOnTop())
    return false;
  auto count = static_cast<int>(m_items.size());
  switch (key)
  {
  case KEY_Up:
    this->highlight(m_current < 0 ? count - 1 : (m_current + count - 1) % count, true);
    return true;
  case KEY_Down:
    this->highlight(m_current < 0 ? 0 : (m_current + 1) % count, true);
    return true;
  case KEY_Enter:
    if (m_current < 0)
      return false;
    m_items[m_current]->activate();
    return true;
  default:
    return false;
  }
}

void MSMenuColumn::choose(MSMenuItem* item)
{
  if (m_busy || !item || !item->getTarget())
    return;
  ms::playSound(Theme::get().clickSound);
  if (m_choose)
    m_choose(item);
}
