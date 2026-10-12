#include "MSLevelList.hpp"

#include "../Theme.hpp"
#include "../ui/MSStyle.hpp"

using namespace geode::prelude;

namespace
{
  constexpr int MOVE_ACTION = 0x4d60;
  constexpr int HIDE_ACTION = 0x4d61;
  constexpr float SCROLL_TIME = 0.14f;
  // The 9-slice's corners are 5.3 pt: thin pieces are made bigger and scaled down
  constexpr float SMALL_PANEL = 2;

  CCNode* bar(ccColor3B color, float width, float height)
  {
    auto node = ms::createPanel(Theme::get().rowSprite, color, 255);
    node->setAnchorPoint({0.5f, 1});
    node->ignoreAnchorPointForPosition(false);
    node->setContentSize(CCSize{width, height} * SMALL_PANEL);
    node->setScale(1 / SMALL_PANEL);
    return node;
  }
} // namespace

MSLevelList* MSLevelList::create(std::vector<std::string> const& names, std::function<void(int)> chosen,
                                 float reserveRight)
{
  auto list = new MSLevelList();
  if (list->init(names, std::move(chosen), reserveRight))
  {
    list->autorelease();
    return list;
  }
  delete list;
  return nullptr;
}

bool MSLevelList::init(std::vector<std::string> const& names, std::function<void(int)> chosen, float reserveRight)
{
  if (!CCMenu::init())
    return false;
  auto const& theme = Theme::get();
  m_onChosen = std::move(chosen);
  this->setPosition({0, 0});
  this->setContentSize({0, 0});
  auto style = MSMenuItem::Style::levels();
  style.reserveRight = reserveRight;
  for (size_t i = 0; i < names.size(); i++)
  {
    auto index = static_cast<int>(i);
    auto row = MSMenuItem::create(
      names[i], nullptr,
      [this, index](MSMenuItem*) {
        if (!this->canUse())
          return;
        this->choose(index, true);
        if (m_onActivate)
          m_onActivate(index);
      },
      theme.levelsRowWidth, style);
    if (!row)
      return false;
    this->addChild(row);
    m_rows.push_back(row);
  }

  // The scroll bar: a dark track the list's height, a white thumb as tall as the share in view
  auto fromTop = CCDirector::get()->getWinSize().height - 320;
  auto top = theme.levelsFirstRowTop + fromTop;
  auto height = theme.levelsRowPitch * (this->visible() - 1) + theme.levelsRowHeight;
  auto x = theme.levelsScrollLeft + theme.levelsScrollWidth / 2;
  m_track = bar(theme.levelsScrollTrackColor, theme.levelsScrollWidth, height);
  m_track->setPosition({x, top});
  this->addChild(m_track, -1);
  auto share = names.empty() ? 1.f : std::min(1.f, static_cast<float>(this->visible()) / names.size());
  m_thumb = bar(theme.levelsScrollThumbColor, theme.levelsScrollWidth, std::max(height * share, 8.f));
  m_thumb->setPosition({x, top});
  this->addChild(m_thumb, -1);
  m_track->setVisible(share < 1);
  m_thumb->setVisible(share < 1);

  this->setMouseEnabled(true);
  m_lastMouse = getMousePos();
  this->layout(false);
  this->scheduleUpdate();
  return true;
}

void MSLevelList::setOnHover(std::function<void(int)> hover) { m_onHover = std::move(hover); }

void MSLevelList::setOnActivate(std::function<void(int)> activate) { m_onActivate = std::move(activate); }

int MSLevelList::visible() const
{
  return std::max(1, std::min(static_cast<int>(Theme::get().levelsVisibleRows), static_cast<int>(m_rows.size())));
}

int MSLevelList::chosen() const { return m_chosen; }

int MSLevelList::offset() const { return m_offset; }

void MSLevelList::setOffset(int offset) { this->scrollTo(offset, false); }

void MSLevelList::setAccessory(int index, CCNode* node)
{
  if (index >= 0 && index < static_cast<int>(m_rows.size()))
    m_rows[index]->setAccessory(node);
}

void MSLevelList::setOnScroll(std::function<void(int)> scrolled) { m_onScroll = std::move(scrolled); }

bool MSLevelList::canUse() { return m_shown && this->isVisible() && !ms::coveredBy(this); }

void MSLevelList::layout(bool animate)
{
  auto const& theme = Theme::get();
  auto fromTop = CCDirector::get()->getWinSize().height - 320;
  auto top = theme.levelsFirstRowTop + fromTop;
  auto visible = this->visible();
  for (int i = 0; i < static_cast<int>(m_rows.size()); i++)
  {
    auto row = m_rows[i];
    auto slot = i - m_offset;
    auto inView = slot >= 0 && slot < visible;
    // Out of view: just past the list's ends, so it slides in from there
    auto clamped = std::clamp(slot, -1, visible);
    CCPoint position{theme.levelsRowLeft, top - theme.levelsRowPitch * clamped - theme.levelsRowHeight / 2};
    row->stopActionByTag(MOVE_ACTION);
    row->stopActionByTag(HIDE_ACTION);
    row->setEnabled(inView && m_shown);
    auto time = animate ? SCROLL_TIME : 0.f;
    if (inView)
    {
      row->setVisible(true);
      row->fadeTo(time, m_shown ? 255 : 0);
    }
    else
    {
      row->fadeTo(time, 0);
      if (time <= 0)
        row->setVisible(false);
      else
      {
        auto hide = CCSequence::createWithTwoActions(CCDelayTime::create(time), CCHide::create());
        hide->setTag(HIDE_ACTION);
        row->runAction(hide);
      }
    }
    if (time <= 0)
      row->setPosition(position);
    else
    {
      auto move = CCEaseOut::create(CCMoveTo::create(time, position), 2);
      move->setTag(MOVE_ACTION);
      row->runAction(move);
    }
  }
  // The thumb at the share scrolled
  auto rows = static_cast<int>(m_rows.size());
  auto height = theme.levelsRowPitch * (visible - 1) + theme.levelsRowHeight;
  auto thumb = m_thumb->getScaledContentSize().height;
  auto travel = std::max(height - thumb, 0.f);
  auto progress = rows > visible ? static_cast<float>(m_offset) / (rows - visible) : 0.f;
  m_thumb->stopAllActions();
  CCPoint at{m_thumb->getPositionX(), top - travel * progress};
  if (animate)
    m_thumb->runAction(CCEaseOut::create(CCMoveTo::create(SCROLL_TIME, at), 2));
  else
    m_thumb->setPosition(at);
}

void MSLevelList::scrollTo(int offset, bool animate)
{
  auto most = std::max(0, static_cast<int>(m_rows.size()) - this->visible());
  offset = std::clamp(offset, 0, most);
  if (offset == m_offset)
    return;
  m_offset = offset;
  m_hover = -1;
  m_lastMouse = CCPoint{-1, -1};
  for (auto row : m_rows)
    row->setHighlighted(false);
  this->layout(animate);
  if (m_onScroll)
    m_onScroll(m_offset);
}

void MSLevelList::choose(int index, bool sound)
{
  if (index < 0 || index >= static_cast<int>(m_rows.size()))
    return;
  // Into view
  if (index < m_offset)
    this->scrollTo(index, true);
  else if (index >= m_offset + this->visible())
    this->scrollTo(index - this->visible() + 1, true);
  if (index == m_chosen)
    return;
  if (m_chosen >= 0)
    m_rows[m_chosen]->setChosen(false);
  m_chosen = index;
  m_rows[m_chosen]->setChosen(true);
  if (sound)
    ms::playSound(Theme::get().clickSound);
  if (m_onChosen)
    m_onChosen(index);
}

void MSLevelList::setShown(bool shown)
{
  if (shown == m_shown)
    return;
  m_shown = shown;
  for (auto row : m_rows)
    row->resetLook();
  if (m_chosen >= 0)
    m_rows[m_chosen]->setChosen(true);
  m_hover = -1;
  m_lastMouse = CCPoint{-1, -1};
  auto time = std::max(Theme::get().pauseDuration, 0.f);
  this->layout(true);
  for (auto node : {m_track, m_thumb})
    node->runAction(CCFadeTo::create(time, shown ? 255 : 0));
}

bool MSLevelList::onKey(enumKeyCodes key)
{
  if (!this->canUse() || m_rows.empty())
    return false;
  auto up = key == KEY_Up || key == CONTROLLER_Up || key == CONTROLLER_LTHUMBSTICK_UP;
  auto down = key == KEY_Down || key == CONTROLLER_Down || key == CONTROLLER_LTHUMBSTICK_DOWN;
  if (!up && !down)
    return false;
  auto next = std::clamp(m_chosen + (up ? -1 : 1), 0, static_cast<int>(m_rows.size()) - 1);
  if (next != m_chosen)
  {
    this->choose(next);
    // The keys' row lit, with its shine, until the mouse moves
    if (m_hover >= 0)
      m_rows[m_hover]->setHighlighted(false);
    m_hover = next;
    m_rows[next]->setHighlighted(true, true);
    ms::playSound(Theme::get().hoverSound);
  }
  return true;
}

void MSLevelList::scrollWheel(float y, float x)
{
  if (!this->canUse())
    return;
  // One row a notch, the wheel's way
  if (y > 0)
    this->scrollTo(m_offset + 1, true);
  else if (y < 0)
    this->scrollTo(m_offset - 1, true);
}

void MSLevelList::update(float dt)
{
  auto mouse = getMousePos();
  if (mouse.equals(m_lastMouse) || m_dragging || !this->canUse())
    return;
  m_lastMouse = mouse;
  // The row under the mouse lights up, with its shine, once as it comes on
  auto local = this->convertToNodeSpace(mouse);
  auto under = -1;
  for (int i = m_offset; i < std::min(m_offset + this->visible(), static_cast<int>(m_rows.size())); i++)
  {
    if (m_rows[i]->boundingBox().containsPoint(local))
    {
      under = i;
      break;
    }
  }
  if (under == m_hover)
    return;
  if (m_hover >= 0)
    m_rows[m_hover]->setHighlighted(false);
  m_hover = under;
  if (m_hover >= 0)
  {
    m_rows[m_hover]->setHighlighted(true, true);
    ms::playSound(Theme::get().hoverSound);
  }
  if (m_onHover)
    m_onHover(m_hover);
}

CCRect MSLevelList::trackRect() const
{
  auto box = m_track->boundingBox();
  // Easier to catch than its 5 pt
  return {box.origin.x - 4, box.origin.y, box.size.width + 8, box.size.height};
}

bool MSLevelList::ccTouchBegan(CCTouch* touch, CCEvent* event)
{
  if (!this->canUse())
    return false;
  if (m_track->isVisible() && this->trackRect().containsPoint(this->convertTouchToNodeSpace(touch)))
  {
    m_dragging = true;
    this->ccTouchMoved(touch, event);
    return true;
  }
  return CCMenu::ccTouchBegan(touch, event);
}

void MSLevelList::ccTouchMoved(CCTouch* touch, CCEvent* event)
{
  if (!m_dragging)
    return CCMenu::ccTouchMoved(touch, event);
  // The thumb's middle follows the touch
  auto box = m_track->boundingBox();
  auto thumb = m_thumb->getScaledContentSize().height;
  auto local = this->convertTouchToNodeSpace(touch);
  auto travel = std::max(box.size.height - thumb, 1.f);
  auto progress = std::clamp((box.getMaxY() - thumb / 2 - local.y) / travel, 0.f, 1.f);
  auto most = std::max(0, static_cast<int>(m_rows.size()) - this->visible());
  this->scrollTo(static_cast<int>(std::lround(progress * most)), true);
}

void MSLevelList::ccTouchEnded(CCTouch* touch, CCEvent* event)
{
  if (m_dragging)
  {
    m_dragging = false;
    return;
  }
  CCMenu::ccTouchEnded(touch, event);
}

void MSLevelList::ccTouchCancelled(CCTouch* touch, CCEvent* event)
{
  if (m_dragging)
  {
    m_dragging = false;
    return;
  }
  CCMenu::ccTouchCancelled(touch, event);
}
