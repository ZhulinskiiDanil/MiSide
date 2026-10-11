#include "MSMenuColumn.hpp"

#include "../Theme.hpp"
#include "MSStyle.hpp"

#include <cmath>

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
  // Everything is placed in the layer's own points
  this->setPosition({0, 0});
  this->setContentSize({0, 0});
  m_lastMouse = getMousePos();
  this->scheduleUpdate();
  return true;
}

MSMenuItem* MSMenuColumn::makeItem(std::string const& text, CCMenuItem* target, float width)
{
  auto item = MSMenuItem::create(text, target, [this](MSMenuItem* item) { this->choose(item); }, width);
  if (item)
    this->addChild(item);
  return item;
}

void MSMenuColumn::addEntry(std::string const& text, CCMenuItem* target)
{
  if (auto item = this->makeItem(text, target, Theme::get().rowWidth))
    m_rows.push_back(item);
}

void MSMenuColumn::setExit(std::string const& text, CCMenuItem* target)
{
  m_exit = this->makeItem(text, target, Theme::get().exitWidth);
}

CCMenuItem* MSMenuColumn::wrap(CCNode* node)
{
  auto item = CCMenuItem::create();
  item->setEnabled(false);
  item->setContentSize(node->getContentSize());
  node->setAnchorPoint({0, 0});
  node->ignoreAnchorPointForPosition(false);
  node->setPosition({0, 0});
  item->addChild(node);
  this->addChild(item);
  return item;
}

void MSMenuColumn::setLogo(MSLogo* logo)
{
  if (!logo)
    return;
  m_logoArt = logo;
  m_logo = this->wrap(logo);
}

void MSMenuColumn::setSubtitle(std::string const& text)
{
  auto const& theme = Theme::get();
  auto subtitle = ms::createText(text, theme.subtitleScale, theme.subtitleColor, theme.titleFont);
  if (!subtitle.node)
    return;
  m_subtitle = this->wrap(subtitle.node);
  m_subtitleText = subtitle.node;
}

void MSMenuColumn::finish()
{
  auto const& theme = Theme::get();
  if (m_logo)
  {
    // Its drawn part as wide as the theme says on the screen, whatever the parents' scale; its drawn bottom left
    // corner at the theme's place
    auto world = this->nodeToWorldTransform();
    m_logoArt->fit(std::sqrt(world.a * world.a + world.b * world.b));
    m_logo->setContentSize(m_logoArt->getContentSize());
    m_logo->setAnchorPoint({0, 0});
    m_logo->setPosition({theme.logoLeft, theme.logoBottom});
  }
  if (m_subtitle)
  {
    m_subtitle->setAnchorPoint({0, 1});
    m_subtitle->setPosition({theme.columnLeft, theme.subtitleTop});
  }

  m_items = m_rows;
  if (m_exit)
    m_items.push_back(m_exit);
  for (size_t i = 0; i < m_rows.size(); i++)
    m_rows[i]->setPosition({theme.columnLeft, theme.firstRowY - theme.rowPitch * i});
  if (m_exit)
  {
    m_exit->setPosition({theme.columnLeft, theme.exitY});
    // It leans away from the mouse around its middle
    m_exit->setDodgeHome({theme.columnLeft + m_exit->getContentWidth() / 2, theme.exitY});
  }
  this->measureArea();

  for (size_t i = 0; i < m_items.size(); i++)
    m_items[i]->playIntro(theme.introStagger * i);
  if (m_subtitleText && theme.introDuration > 0)
  {
    ms::setOpacity(m_subtitleText, 0);
    m_subtitleText->runAction(CCFadeTo::create(theme.introDuration, 255));
  }
}

void MSMenuColumn::measureArea()
{
  auto const& theme = Theme::get();
  CCRect area;
  auto any = false;
  for (auto child : CCArrayExt<CCNode*>(this->getChildren()))
  {
    if (!child->isVisible())
      continue;
    // Where the item rests; Quit with the room it may lean into
    auto box = child->boundingBox();
    if (child == m_exit)
    {
      auto lean = theme.exitDodgeMax * 1.25f;
      box = {box.origin.x - lean, box.origin.y - lean, box.size.width + lean * 2, box.size.height + lean * 2};
    }
    if (!any)
      area = box;
    else
    {
      auto minX = std::min(area.getMinX(), box.getMinX());
      auto minY = std::min(area.getMinY(), box.getMinY());
      area = {minX, minY, std::max(area.getMaxX(), box.getMaxX()) - minX,
              std::max(area.getMaxY(), box.getMaxY()) - minY};
    }
    any = true;
  }
  m_area = area;
}

CCRect MSMenuColumn::getArea()
{
  // Measured once: Quit leaning away doesn't move what's kept out of the menu's way
  return CCRectApplyAffineTransform(m_area, this->nodeToParentTransform());
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

void MSMenuColumn::update(float dt)
{
  auto mouse = getMousePos();
  auto moved = !mouse.equals(m_lastMouse);
  m_lastMouse = mouse;
  if (moved)
    m_mouseActive = true;
  auto usable = !m_busy && this->isOnTop();
  auto local = this->convertToNodeSpace(mouse);

  // Quit leans away from the mouse, every frame (it springs back with no mouse to lean from)
  if (m_exit)
    m_exit->stepDodge(dt, usable && m_mouseActive ? &local : nullptr);

  if (!moved || !usable)
    return;
  auto under = -1;
  for (size_t i = 0; i < m_items.size(); i++)
  {
    if (m_items[i]->boundingBox().containsPoint(local))
    {
      under = static_cast<int>(i);
      break;
    }
  }
  if (under == m_hover)
    return;
  m_hover = under;
  // The shine on coming onto a row; not when leaving lights the keyboard's row again
  this->showHighlight(m_hover >= 0);
}

void MSMenuColumn::showHighlight(bool shine)
{
  auto count = static_cast<int>(m_items.size());
  auto index = m_hover >= 0 ? m_hover : m_keyboard;
  if (index >= count)
    index = -1;
  if (index == m_shown)
    return;
  if (m_shown >= 0 && m_shown < count)
    m_items[m_shown]->setHighlighted(false);
  m_shown = index;
  if (m_shown < 0)
    return;
  m_items[m_shown]->setHighlighted(true, shine);
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
  case KEY_Down:
  {
    // From what's highlighted now, the mouse's row too; the keys take over until the mouse moves again
    auto step = key == KEY_Up ? count - 1 : 1;
    m_keyboard = m_shown < 0 ? (key == KEY_Up ? count - 1 : 0) : (m_shown + step) % count;
    m_hover = -1;
    // The keys in use: Quit stops leaning away until the mouse moves again
    m_mouseActive = false;
    this->showHighlight(true);
    return true;
  }
  case KEY_Enter:
    if (m_shown < 0)
      return false;
    m_items[m_shown]->activate();
    return true;
  default:
    return false;
  }
}

void MSMenuColumn::choose(MSMenuItem* item)
{
  // Busy from the first press until the transition ends: a second click or Enter can't press it again
  if (m_busy || !item || !item->getTarget())
    return;
  ms::playSound(Theme::get().clickSound);
  if (m_choose)
    m_choose(item);
}
