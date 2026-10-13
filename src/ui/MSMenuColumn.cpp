#include "MSMenuColumn.hpp"

#include "../Theme.hpp"
#include "MSStyle.hpp"

#include <cmath>
#include <set>
#include <typeinfo>

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
    auto at = m_exit->convertToWorldSpace({0, 0});
    log::debug("Column: {} rows, EXIT at {:.0f},{:.0f} on screen ({} visible)", m_rows.size(), at.x, at.y,
               m_exit->isVisible() ? "" : "not ");
  }
  this->measureArea();
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

void MSMenuColumn::setBusy(bool busy)
{
  m_busy = busy;
  if (busy)
    this->notifyExitHover(false);
}

void MSMenuColumn::setOnExitHover(std::function<void(bool)> callback)
{
  m_onExitHover = std::move(callback);
}

void MSMenuColumn::notifyExitHover(bool hovered)
{
  if (m_exitHovered == hovered)
    return;
  m_exitHovered = hovered;
  if (m_onExitHover)
    m_onExitHover(hovered);
}

MSMenuItem* MSMenuColumn::getExit() const { return m_exit; }

ms::wave::Steps MSMenuColumn::pageSteps()
{
  ms::wave::Steps steps;
  if (ms::wave::shows(m_subtitle))
    steps.push_back({{m_subtitle}});
  for (auto row : m_rows)
  {
    if (ms::wave::shows(row))
      steps.push_back({{row}});
  }
  if (ms::wave::shows(m_exit))
    steps.push_back({{m_exit}});
  return steps;
}

bool MSMenuColumn::isConfirming() const { return m_confirming; }

void MSMenuColumn::setConfirming(bool confirming)
{
  if (confirming == m_confirming)
    return;
  m_confirming = confirming;
  this->notifyExitHover(false);
  auto const& theme = Theme::get();
  if (confirming)
  {
    m_keysOnExit = m_hover < 0 && m_shown >= 0 && m_items[m_shown] == m_exit;
  }
  m_shown = -1;
  m_hover = -1;
  // Nothing is chosen from the first frame of going; each one back at rest, its shine stopped
  for (auto item : m_items)
  {
    item->setEnabled(!confirming);
    item->resetLook();
  }
  // MAIN MENU and the rows go to the right as the question comes, and back from the left, as pages do (the logo
  // stays)
  if (confirming)
  {
    ms::wave::out(this->pageSteps());
    return;
  }
  ms::wave::in(this->pageSteps(), theme.pageInDelay);
  // Back to the menu as it was: the keys' Quit chosen again, or the mouse's row once it moves
  m_keyboard = -1;
  if (m_keysOnExit && m_exit)
  {
    m_keyboard = static_cast<int>(std::find(m_items.begin(), m_items.end(), m_exit) - m_items.begin());
    this->showHighlight(false);
  }
  m_lastMouse = CCPoint{-1, -1};
}

bool MSMenuColumn::isOnTop()
{
  // Why hover is paused, once a session each: if it never works, the log names what's in the way
  static std::set<std::string> s_logged;
  auto pausedBy = [](std::string const& why) {
    if (s_logged.insert(why).second)
      log::info("Hover and the arrow keys are paused: {}", why);
    return false;
  };

  auto why = ms::coveredBy(this);
  if (!why)
    return true;
  // A scene transition: not worth a log, it happens every time
  return why->empty() ? false : pausedBy(*why);
}

void MSMenuColumn::update(float dt)
{
  auto mouse = getMousePos();
  auto moved = !mouse.equals(m_lastMouse);
  m_lastMouse = mouse;
  if (moved)
    m_mouseActive = true;
  auto usable = !m_busy && !m_confirming && this->isOnTop();
  auto local = this->convertToNodeSpace(mouse);
  // The keys' row while they're in use (until the mouse moves): Mita follows it as she follows the mouse
  MSMenuItem* keyed = nullptr;
  if (usable && !m_mouseActive && m_shown >= 0 && m_shown < static_cast<int>(m_items.size()))
    keyed = m_items[m_shown];
  this->notifyExitHover(usable && m_exit &&
                        (m_mouseActive ? m_exit->boundingBox().containsPoint(local) : keyed == m_exit));

  // Quit leans away from the mouse, every frame (it springs back with no mouse to lean from); not while a page's
  // wave moves it, or it's out
  if (m_exit && m_exit->isVisible() && !ms::wave::moving(m_exit))
    m_exit->stepDodge(dt, usable && m_mouseActive ? &local : nullptr);

  if (m_onGazeTarget)
  {
    std::optional<CCPoint> target;
    auto middle = [](MSMenuItem* item) {
      auto size = item->getContentSize();
      return item->convertToWorldSpace({size.width / 2, size.height / 2});
    };
    if (keyed)
      target = middle(keyed);
    else if (usable)
      for (auto item : m_items)
        if (item->isVisible() && item->isEnabled() && item->boundingBox().containsPoint(local))
        {
          target = middle(item);
          break;
        }
    m_onGazeTarget(target);
  }
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
  if (m_items.empty() || m_busy || m_confirming || !this->isOnTop())
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
    this->notifyExitHover(false);
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
  if (m_busy || m_confirming || !item || !item->getTarget())
    return;
  ms::playSound(Theme::get().clickSound);
  if (m_choose)
    m_choose(item);
}
