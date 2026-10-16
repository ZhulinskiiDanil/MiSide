#include "MSAchievementList.hpp"

#include "../Theme.hpp"
#include "../ui/MSStyle.hpp"

#include <cmath>

using namespace geode::prelude;

namespace
{
  // The 9-slice's corners are 5.3 pt: thin pieces are made bigger and scaled down
  constexpr float SMALL_PANEL = 4;
  // A press that moves this far scrolls instead of clicking
  constexpr float DRAG_START = 4;
  // Room in a row: around the icon, and between it and the title
  constexpr float ICON_MARGIN = 2.2f;
  constexpr float TITLE_GAP = 3;
  constexpr float SIDE_PADDING = 4;
  // The check's room at the right end of an earned row
  constexpr float CHECK_ROOM = 9;
  // The icon's white frame, and the picture inside its dark back
  constexpr float FRAME = 1.2f;
  constexpr float ICON_FILL = 0.78f;
  // The details: their padding, the gaps between their lines and the progress bar's height
  constexpr float DETAILS_PADDING = 4;
  constexpr float DETAILS_GAP = 2.5f;
  constexpr float BAR_HEIGHT = 2;
  // The title font's capitals: their middle this far over the label's, at scale 1
  constexpr float TITLE_RAISE = 2.82f;
  // The wheel scrolls this many rows a notch
  constexpr float WHEEL_ROWS = 1.5f;

  float mix(float from, float to, float t) { return from + (to - from) * t; }

  ccColor3B mix(ccColor3B from, ccColor3B to, float t)
  {
    auto channel = [t](GLubyte a, GLubyte b) { return static_cast<GLubyte>(std::lround(mix(a, b, t))); };
    return {channel(from.r, to.r), channel(from.g, to.g), channel(from.b, to.b)};
  }

  // A panel with its top left corner at the origin; a thin one drawn bigger and scaled down
  CCNode* panel(ccColor3B color, GLubyte opacity, CCSize size, float scale = 1)
  {
    auto node = ms::createPanel(Theme::get().rowSprite, color, opacity);
    node->setAnchorPoint({0, 1});
    node->ignoreAnchorPointForPosition(false);
    node->setContentSize(size * scale);
    node->setScale(1 / scale);
    return node;
  }


  // Upper case, the Latin letters (GD's titles are English)
  std::string upper(std::string text)
  {
    for (auto& c : text)
    {
      if (c >= 'a' && c <= 'z')
        c = static_cast<char>(c - 'a' + 'A');
    }
    return text;
  }

  char const* rewardName(UnlockType type, bool english)
  {
    switch (type)
    {
      case UnlockType::Cube: return english ? "CUBE" : "КУБ";
      case UnlockType::Col1: return english ? "PRIMARY COLOR" : "ОСНОВНОЙ ЦВЕТ";
      case UnlockType::Col2: return english ? "SECONDARY COLOR" : "ВТОРОЙ ЦВЕТ";
      case UnlockType::Ship: return english ? "SHIP" : "КОРАБЛЬ";
      case UnlockType::Ball: return english ? "BALL" : "ШАР";
      case UnlockType::Bird: return english ? "UFO" : "НЛО";
      case UnlockType::Dart: return english ? "WAVE" : "ВОЛНА";
      case UnlockType::Robot: return english ? "ROBOT" : "РОБОТ";
      case UnlockType::Spider: return english ? "SPIDER" : "ПАУК";
      case UnlockType::Streak: return english ? "TRAIL" : "СЛЕД";
      case UnlockType::Death: return english ? "DEATH EFFECT" : "ЭФФЕКТ СМЕРТИ";
      case UnlockType::Swing: return english ? "SWING" : "СВИНГ";
      case UnlockType::Jetpack: return english ? "JETPACK" : "ДЖЕТПАК";
      case UnlockType::ShipFire: return english ? "SHIP FIRE" : "ОГОНЬ КОРАБЛЯ";
      default: return english ? "ITEM" : "ПРЕДМЕТ";
    }
  }

  // The reward as GD's achievement cell shows it (GJItemIcon, its colours), dark while it isn't earned; no reward:
  // GD's achievement picture
  CCNode* rewardIcon(MSAchievement const& achievement, float size)
  {
    CCNode* icon = nullptr;
    if (achievement.hasReward)
      icon = GJItemIcon::create(achievement.type, achievement.item, {175, 175, 175}, {255, 255, 255},
                                !achievement.earned, false, false, {255, 255, 255});
    if (!icon)
      icon = CCSprite::createWithSpriteFrameName("GJ_achImage_001.png");
    if (!icon)
      return nullptr;
    auto own = icon->getContentSize();
    auto biggest = std::max(own.width, own.height);
    icon->setScale(size / (biggest > 0 ? biggest : 30.f));
    icon->setAnchorPoint({0.5f, 0.5f});
    icon->ignoreAnchorPointForPosition(false);
    return icon;
  }

  // The scissor box: only what's inside this node's own rectangle draws (the rows scrolled past the list's ends)
  class Clip : public CCNode
  {
  public:
    static Clip* create()
    {
      auto clip = new Clip();
      clip->init();
      clip->autorelease();
      return clip;
    }

    void visit() override
    {
      if (!m_bVisible)
        return;
      auto from = this->convertToWorldSpace({0, 0});
      auto to = this->convertToWorldSpace(CCPoint{m_obContentSize.width, m_obContentSize.height});
      auto was = glIsEnabled(GL_SCISSOR_TEST);
      GLint box[4] = {0, 0, 0, 0};
      if (was)
        glGetIntegerv(GL_SCISSOR_BOX, box);
      glEnable(GL_SCISSOR_TEST);
      CCEGLView::sharedOpenGLView()->setScissorInPoints(from.x, from.y, to.x - from.x, to.y - from.y);
      CCNode::visit();
      if (was)
        glScissor(box[0], box[1], box[2], box[3]);
      else
        glDisable(GL_SCISSOR_TEST);
    }
  };
} // namespace

// ! --- A row --- !

MSAchievementRow* MSAchievementRow::create(float width)
{
  auto row = new MSAchievementRow();
  if (row->init(width))
  {
    row->autorelease();
    return row;
  }
  delete row;
  return nullptr;
}

bool MSAchievementRow::init(float width)
{
  if (!CCNode::init())
    return false;
  m_width = width;
  return true;
}

int MSAchievementRow::index() const { return m_index; }

float MSAchievementRow::height() const { return Theme::get().achievementsRowHeight + m_extra; }

bool MSAchievementRow::isLit() const { return m_litTarget; }

void MSAchievementRow::bind(MSAchievement const& achievement, int index, bool english)
{
  auto const& theme = Theme::get();
  m_achievement = achievement;
  m_index = index;
  m_english = english;
  m_extra = 0;
  m_lit = 0;
  m_litTarget = false;
  m_shine = nullptr;
  m_shineGlow = nullptr;
  m_shineTried = false;
  m_shinePass.stop();
  m_details = nullptr;
  m_detailsHeight = 0;
  this->removeAllChildrenWithCleanup(true);

  auto height = theme.achievementsRowHeight;
  m_plate = panel(theme.achievementsRowColor, theme.achievementsRowOpacity, {m_width, height});
  this->addChild(m_plate);
  // The closed row's content, at its top
  m_header = CCNode::create();
  this->addChild(m_header, 2);

  // The icon: a white frame, a dark back, the reward
  auto size = theme.achievementsIconSize;
  CCPoint middle{ICON_MARGIN + size / 2, -height / 2};
  auto frame = panel(theme.achievementsIconFrameColor, 255, {size, size});
  frame->setPosition(middle + CCPoint{-size / 2, size / 2});
  m_header->addChild(frame);
  auto inner = size - FRAME * 2;
  auto back = panel(theme.achievementsIconBackColor, 255, {inner, inner});
  back->setPosition(middle + CCPoint{-inner / 2, inner / 2});
  m_header->addChild(back);
  if (auto icon = rewardIcon(achievement, inner * ICON_FILL))
  {
    icon->setPosition(middle);
    m_header->addChild(icon);
  }
  if (!achievement.earned)
  {
    // GD's lock, small, in the frame's corner
    if (auto lock = CCSprite::createWithSpriteFrameName("GJ_lock_001.png"))
    {
      auto own = lock->getContentSize();
      if (own.height > 0)
        lock->setScale(size * 0.42f / own.height);
      lock->setPosition(middle + CCPoint{size / 2 - 2.5f, -size / 2 + 3});
      m_header->addChild(lock);
    }
  }

  // The title in the room right of the icon, made smaller when it doesn't fit
  auto left = ICON_MARGIN + size + TITLE_GAP;
  auto right = m_width - (achievement.earned ? CHECK_ROOM : SIDE_PADDING);
  auto room = right - left;
  m_titleText = upper(achievement.title);
  m_titleScale = theme.achievementsTitleScale;
  auto color = achievement.earned ? theme.achievementsTitleColor : theme.achievementsTitleLockedColor;
  auto text = ms::createText(m_titleText, m_titleScale, color, theme.titleFont);
  if (text.node && text.node->getContentWidth() > room && room > 0)
  {
    m_titleScale *= room / text.node->getContentWidth();
    text = ms::createText(m_titleText, m_titleScale, color, theme.titleFont);
  }
  if (text.node)
  {
    text.node->setPosition({left + (room - text.node->getContentWidth()) / 2,
                            -height / 2 - TITLE_RAISE * m_titleScale});
    if (!achievement.earned)
      ms::setOpacity(text.node, theme.achievementsTitleLockedOpacity);
    m_header->addChild(text.node, 1);
  }
  m_title = text.node;
  m_titleLabel = text.label;

  // Earned: a small check at the right end
  if (achievement.earned)
  {
    auto check = CCDrawNode::create();
    auto tint = theme.achievementsCheckColor;
    ccColor4F ink{tint.r / 255.f, tint.g / 255.f, tint.b / 255.f, 1};
    CCPoint at{m_width - CHECK_ROOM / 2 - 0.5f, -height / 2};
    check->drawSegment(at + CCPoint{-2.2f, 0.1f}, at + CCPoint{-0.7f, -1.6f}, 0.55f, ink);
    check->drawSegment(at + CCPoint{-0.7f, -1.6f}, at + CCPoint{2.3f, 2}, 0.55f, ink);
    m_header->addChild(check, 1);
  }
  this->apply();
}

void MSAchievementRow::buildDetails()
{
  if (m_details)
    return;
  auto const& theme = Theme::get();
  auto const& a = m_achievement;
  auto details = CCNodeRGBA::create();
  details->setCascadeOpacityEnabled(true);
  auto width = m_width - DETAILS_PADDING * 2;
  auto y = 0.f;

  // The condition, as GD words it for its status
  if (auto text = ms::createWrapped(a.description.empty() ? "?" : a.description, theme.achievementsTextScale,
                          theme.achievementsTextColor, width))
  {
    text->setPosition({DETAILS_PADDING, y});
    details->addChild(text);
    y -= text->getScaledContentSize().height + DETAILS_GAP;
  }

  // The status on the left, the reward on the right
  std::string status;
  if (a.earned)
    status = m_english ? "EARNED" : "ПОЛУЧЕНО";
  else if (a.percent > 0)
    status = fmt::format("{} {}%", m_english ? "PROGRESS" : "ПРОГРЕСС", a.percent);
  else
    status = m_english ? "NOT EARNED" : "НЕ ПОЛУЧЕНО";
  auto lineHeight = 0.f;
  if (auto text = ms::createText(status, theme.achievementsTextScale, theme.achievementsStatusColor, theme.font,
                                 ms::Shadow{}).node)
  {
    lineHeight = text->getContentHeight();
    text->setPosition({DETAILS_PADDING, y - lineHeight / 2});
    details->addChild(text);
  }
  if (a.hasReward)
  {
    auto reward = fmt::format("{}: {} #{}", m_english ? "REWARD" : "НАГРАДА", rewardName(a.type, m_english), a.item);
    if (auto text = ms::createText(reward, theme.achievementsTextScale, theme.achievementsTextColor, theme.font,
                                   ms::Shadow{}).node)
    {
      lineHeight = std::max(lineHeight, text->getContentHeight());
      text->setPosition({m_width - DETAILS_PADDING - text->getContentWidth(), y - lineHeight / 2});
      details->addChild(text);
    }
  }
  y -= lineHeight;

  // Partway: how far, as a bar
  if (!a.earned && a.percent > 0)
  {
    y -= DETAILS_GAP;
    auto track = panel(theme.achievementsProgressTrackColor, 255, {width, BAR_HEIGHT}, SMALL_PANEL);
    track->setPosition({DETAILS_PADDING, y});
    details->addChild(track);
    auto share = std::clamp(a.percent / 100.f, 0.f, 1.f);
    auto fill = panel(theme.achievementsProgressFillColor, 255, {std::max(width * share, BAR_HEIGHT), BAR_HEIGHT},
                      SMALL_PANEL);
    fill->setPosition({DETAILS_PADDING, y});
    details->addChild(fill);
    y -= BAR_HEIGHT;
  }

  m_detailsHeight = -y + DETAILS_PADDING;
  details->setPosition({0, -theme.achievementsRowHeight + 1});
  details->setOpacity(0);
  this->addChild(details, 1);
  m_details = details;
}

float MSAchievementRow::openExtra()
{
  this->buildDetails();
  return m_detailsHeight;
}

void MSAchievementRow::setOpen(float extra, float shown)
{
  if (extra > 0)
    this->buildDetails();
  m_extra = std::max(extra, 0.f);
  if (m_plate)
    m_plate->setContentSize({m_width, Theme::get().achievementsRowHeight + m_extra});
  if (m_details)
  {
    m_details->setVisible(shown > 0);
    static_cast<CCNodeRGBA*>(m_details)->setOpacity(static_cast<GLubyte>(std::clamp(shown, 0.f, 1.f) * 255));
  }
}

void MSAchievementRow::setLit(bool lit, bool shine)
{
  if (m_litTarget == lit)
    return;
  m_litTarget = lit;
  if (lit && shine)
  {
    auto const& theme = Theme::get();
    // The shine made the first time it's needed for this title: a few rows are hovered, not hundreds
    if (!m_shineTried && m_title && theme.shineEnabled && theme.shineDuration > 0 && theme.shineIntensity > 0)
    {
      m_shineTried = true;
      auto pair = MSShine::create(m_titleText, m_titleScale, theme.titleFont);
      if (pair.core && pair.glow)
      {
        m_shine = pair.core;
        m_shineGlow = pair.glow;
        m_title->addChild(m_shineGlow, -1);
        m_title->addChild(m_shine, 1);
      }
    }
    if (m_shine)
      m_shinePass.start();
  }
  else if (!lit)
    m_shinePass.leave();
}

void MSAchievementRow::step(float dt)
{
  auto const& theme = Theme::get();
  auto target = m_litTarget ? 1.f : 0.f;
  if (m_lit != target)
  {
    auto speed = theme.hoverDuration > 0 ? dt / theme.hoverDuration : 1.f;
    m_lit = target > m_lit ? std::min(target, m_lit + speed) : std::max(target, m_lit - speed);
    this->apply();
  }
  if (m_shine)
  {
    m_shinePass.advance(dt, theme.shineDuration, theme.shineFadeOut);
    auto strength = m_shinePass.active() ? theme.shineIntensity * m_shinePass.fade() : 0.f;
    auto travel = theme.shineBandWidth + 2 * m_shine->margin();
    auto center = m_shinePass.center(theme.shineDuration, travel);
    m_shine->setPass(center, strength);
    m_shineGlow->setPass(center, strength);
  }
}

void MSAchievementRow::apply()
{
  if (!m_plate)
    return;
  auto const& theme = Theme::get();
  auto earned = m_achievement.earned;
  auto color = earned ? theme.achievementsRowColor : theme.achievementsRowLockedColor;
  float opacity = earned ? theme.achievementsRowOpacity : theme.achievementsRowLockedOpacity;
  ms::setColor(m_plate, mix(color, theme.achievementsRowHoverColor, m_lit));
  ms::setOpacity(m_plate, static_cast<GLubyte>(std::lround(mix(opacity, theme.achievementsRowHoverOpacity, m_lit))));
  // A title not earned brightens a little when lit
  if (m_titleLabel && !earned)
    ms::setColor(m_titleLabel, mix(theme.achievementsTitleLockedColor, theme.achievementsTitleColor, m_lit * 0.4f));
}

// ! --- The list --- !

MSAchievementList* MSAchievementList::create(std::vector<MSAchievement> achievements, bool english)
{
  auto list = new MSAchievementList();
  if (list->init(std::move(achievements), english))
  {
    list->autorelease();
    return list;
  }
  delete list;
  return nullptr;
}

bool MSAchievementList::init(std::vector<MSAchievement> achievements, bool english)
{
  if (!CCMenu::init())
    return false;
  auto const& theme = Theme::get();
  m_achievements = std::move(achievements);
  m_english = english;
  this->setPosition({0, 0});
  this->setContentSize({0, 0});

  // The rows are cut at the list's top and bottom; sideways there's room for the shine's glow, and for the rows
  // sliding past the bar as the page goes
  auto view = this->viewRect();
  auto clip = Clip::create();
  clip->setPosition({0, view.origin.y});
  clip->setContentSize({std::max(theme.achievementsScrollLeft - 1, CCDirector::get()->getWinSize().width),
                        view.size.height});
  this->addChild(clip);
  m_clip = clip;

  // As many rows as can be in view at once, and one more at each end
  auto count = static_cast<int>(std::ceil(view.size.height / this->pitch())) + 2;
  count = std::min(count, static_cast<int>(m_achievements.size()));
  for (int i = 0; i < count; i++)
  {
    auto row = MSAchievementRow::create(theme.achievementsRowWidth);
    if (!row)
      return false;
    row->setVisible(false);
    clip->addChild(row);
    m_rows.push_back(row);
  }

  // The scroll bar: a track the list's height, a white thumb as tall as the share in view
  auto x = theme.achievementsScrollLeft;
  auto width = theme.achievementsScrollWidth;
  m_track = panel(theme.achievementsScrollTrackColor, 255, {width, view.size.height}, SMALL_PANEL);
  m_track->setPosition({x, view.getMaxY()});
  this->addChild(m_track, -1);
  m_thumb = panel(theme.achievementsScrollThumbColor, 255, {width, 8}, SMALL_PANEL);
  m_thumb->setPosition({x, view.getMaxY()});
  this->addChild(m_thumb, -1);

  this->setMouseEnabled(true);
  m_lastMouse = getMousePos();
  this->layout();
  this->scheduleUpdate();
  return true;
}

float MSAchievementList::pitch() const
{
  auto const& theme = Theme::get();
  return theme.achievementsRowHeight + theme.achievementsRowGap;
}

CCRect MSAchievementList::viewRect() const
{
  auto const& theme = Theme::get();
  auto fromTop = CCDirector::get()->getWinSize().height - 320;
  auto top = theme.achievementsListTop + fromTop;
  auto bottom = std::min(theme.achievementsListBottom + fromTop, top - theme.achievementsRowHeight);
  return {theme.achievementsListLeft, bottom, theme.achievementsRowWidth, top - bottom};
}

float MSAchievementList::viewHeight() const { return this->viewRect().size.height; }

float MSAchievementList::top(int index) const
{
  auto y = index * this->pitch();
  if (m_open >= 0 && index > m_open)
    y += m_openExtra;
  if (m_closing >= 0 && index > m_closing)
    y += m_closingExtra;
  return y;
}

float MSAchievementList::rowHeight(int index) const
{
  auto height = Theme::get().achievementsRowHeight;
  if (index == m_open)
    height += m_openExtra;
  if (index == m_closing)
    height += m_closingExtra;
  return height;
}

float MSAchievementList::contentHeight() const
{
  auto count = static_cast<int>(m_achievements.size());
  if (count == 0)
    return 0;
  return this->top(count - 1) + this->rowHeight(count - 1);
}

float MSAchievementList::maxScroll() const { return std::max(0.f, this->contentHeight() - this->viewHeight()); }

bool MSAchievementList::canUse() { return m_active && this->isVisible() && !ms::coveredBy(this); }

void MSAchievementList::setActive(bool active)
{
  m_active = active;
  if (!active)
    this->light(-1, false);
}

MSAchievementRow* MSAchievementList::rowFor(int index) const
{
  for (auto row : m_rows)
  {
    if (row->isVisible() && row->index() == index)
      return row;
  }
  return nullptr;
}

void MSAchievementList::layout()
{
  auto count = static_cast<int>(m_achievements.size());
  auto view = this->viewHeight();
  auto const& theme = Theme::get();
  // The first one in view: from its place without the open rows, then on to the first that reaches the view
  auto first = std::clamp(static_cast<int>((m_scroll - m_openExtra - m_closingExtra) / this->pitch()) - 1, 0,
                          std::max(count - 1, 0));
  while (first < count - 1 && this->top(first) + this->rowHeight(first) < m_scroll)
    first++;
  auto last = first;
  while (last + 1 < count && this->top(last + 1) < m_scroll + view)
    last++;

  // The rows already showing one of these keep it; the others are given the ones missing
  std::vector<bool> shown(static_cast<size_t>(std::max(last - first + 1, 0)), false);
  std::vector<MSAchievementRow*> free;
  for (auto row : m_rows)
  {
    auto index = row->index();
    if (row->isVisible() && index >= first && index <= last && !shown[index - first])
      shown[index - first] = true;
    else
      free.push_back(row);
  }
  for (int index = first; index <= last && count > 0; index++)
  {
    if (shown[index - first])
      continue;
    if (free.empty())
      break;
    auto row = free.back();
    free.pop_back();
    row->bind(m_achievements[index], index, m_english);
    row->setVisible(true);
    // Lit by the mouse or the keys: lit again (no new shine)
    if (index == m_lit)
      row->setLit(true, false);
  }
  for (auto row : free)
    row->setVisible(false);

  // In place, open as much as they are now
  auto full = [&](int index) { return index == m_open ? m_openFull : index == m_closing ? m_closingFull : 0.f; };
  for (auto row : m_rows)
  {
    if (!row->isVisible())
      continue;
    auto index = row->index();
    auto extra = this->rowHeight(index) - theme.achievementsRowHeight;
    auto most = full(index);
    auto shown = most > 0 ? extra / most : 0.f;
    row->setOpen(extra, shown * shown);
    row->setPosition({theme.achievementsListLeft, view - (this->top(index) - m_scroll)});
  }

  // The thumb: as tall as the share in view, at the share scrolled
  auto content = this->contentHeight();
  auto share = content > 0 ? std::min(1.f, view / content) : 1.f;
  auto thumb = std::max(view * share, 8.f);
  m_thumb->setContentSize(CCSize{theme.achievementsScrollWidth, thumb} * SMALL_PANEL);
  auto travel = std::max(view - thumb, 0.f);
  auto most = this->maxScroll();
  auto progress = most > 0 ? std::clamp(m_scroll / most, 0.f, 1.f) : 0.f;
  m_thumb->setPositionY(this->viewRect().getMaxY() - travel * progress);
  m_track->setVisible(share < 1);
  m_thumb->setVisible(share < 1);
}

void MSAchievementList::reveal(int index)
{
  if (index < 0)
    return;
  auto top = this->top(index);
  auto bottom = top + this->rowHeight(index);
  // Open: all of it, as it will be
  if (index == m_open)
    bottom = top + Theme::get().achievementsRowHeight + m_openFull;
  if (top < m_target)
    m_target = top;
  else if (bottom > m_target + this->viewHeight())
    m_target = std::min(top, bottom - this->viewHeight());
  m_target = std::clamp(m_target, 0.f, std::max(this->maxScroll(), bottom - this->viewHeight()));
}

void MSAchievementList::toggle(int index)
{
  if (index < 0 || index >= static_cast<int>(m_achievements.size()))
    return;
  ms::playSound(Theme::get().clickSound);
  m_anchor = index;
  if (index == m_open)
  {
    // Closing it
    m_closing = m_open;
    m_closingExtra = m_openExtra;
    m_closingFull = m_openFull;
    m_open = -1;
    m_openExtra = 0;
    m_openFull = 0;
    return;
  }
  // The open one closes (one still closing is closed at once)
  if (m_open >= 0)
  {
    m_closing = m_open;
    m_closingExtra = m_openExtra;
    m_closingFull = m_openFull;
  }
  else if (index == m_closing)
  {
    // Opening again the one closing: from where it is
    m_open = index;
    m_openExtra = m_closingExtra;
    m_openFull = m_closingFull;
    m_closing = -1;
    m_closingExtra = 0;
    m_closingFull = 0;
    this->reveal(index);
    return;
  }
  m_open = index;
  m_openExtra = 0;
  m_openFull = 0;
  if (auto row = this->rowFor(index))
    m_openFull = row->openExtra();
  this->reveal(index);
}

int MSAchievementList::rowAt(CCPoint local)
{
  if (!this->viewRect().containsPoint(local))
    return -1;
  for (auto row : m_rows)
  {
    if (!row->isVisible())
      continue;
    auto at = m_clip->convertToWorldSpace(row->getPosition());
    auto inList = this->convertToNodeSpace(at);
    CCRect box{inList.x, inList.y - row->height(), Theme::get().achievementsRowWidth, row->height()};
    if (box.containsPoint(local))
      return row->index();
  }
  return -1;
}

void MSAchievementList::light(int index, bool shine)
{
  if (index == m_lit)
    return;
  if (auto row = this->rowFor(m_lit))
    row->setLit(false, false);
  m_lit = index;
  if (auto row = this->rowFor(index))
  {
    row->setLit(true, shine);
    ms::playSound(Theme::get().hoverSound);
  }
}

bool MSAchievementList::onKey(enumKeyCodes key)
{
  if (!this->canUse() || m_achievements.empty())
    return false;
  auto up = key == KEY_Up || key == CONTROLLER_Up || key == CONTROLLER_LTHUMBSTICK_UP;
  auto down = key == KEY_Down || key == CONTROLLER_Down || key == CONTROLLER_LTHUMBSTICK_DOWN;
  auto enter = key == KEY_Enter || key == KEY_Space || key == CONTROLLER_A;
  if (enter)
  {
    if (m_cursor >= 0)
      this->toggle(m_cursor);
    return m_cursor >= 0;
  }
  if (!up && !down)
    return false;
  auto count = static_cast<int>(m_achievements.size());
  auto start = m_cursor >= 0 ? m_cursor : (m_lit >= 0 ? m_lit : (up ? count : -1));
  auto next = std::clamp(start + (up ? -1 : 1), 0, count - 1);
  m_cursor = next;
  this->reveal(next);
  // The keys' row lit, with its shine, until the mouse moves
  m_lastMouse = getMousePos();
  this->light(next, true);
  return true;
}

bool MSAchievementList::scrollWheel(float y, float x)
{
  this->scrollWheelImpl(y, x);
  return true;
}

void MSAchievementList::scrollWheelImpl(float y, float x)
{
  if (!this->canUse() || y == 0)
    return;
  m_target = std::clamp(m_target + (y > 0 ? 1 : -1) * this->pitch() * WHEEL_ROWS, 0.f, this->maxScroll());
}

ms::wave::Steps MSAchievementList::pageSteps()
{
  ms::wave::Steps steps;
  ms::wave::Step bar;
  bar.slide = false;
  for (auto node : {m_track, m_thumb})
  {
    if (ms::wave::shows(node))
      bar.nodes.push_back(node);
  }
  if (!bar.nodes.empty())
    steps.push_back(std::move(bar));
  for (auto row : m_rows)
  {
    if (ms::wave::shows(row))
      steps.push_back({{row}});
  }
  return steps;
}

void MSAchievementList::update(float dt)
{
  auto const& theme = Theme::get();
  dt = std::min(dt, 0.1f);

  // The rows opening and closing, the row clicked kept where it is on screen
  auto anchorBefore = m_anchor >= 0 ? this->top(m_anchor) : 0.f;
  auto speed = theme.achievementsExpandDuration > 0 ? dt / theme.achievementsExpandDuration : 1.f;
  auto changed = false;
  if (m_open >= 0 && m_openExtra < m_openFull)
  {
    m_openExtra = std::min(m_openFull, m_openExtra + m_openFull * speed);
    changed = true;
  }
  if (m_closing >= 0)
  {
    m_closingExtra = std::max(0.f, m_closingExtra - std::max(m_closingFull, 1.f) * speed);
    if (m_closingExtra <= 0)
    {
      m_closing = -1;
      m_closingFull = 0;
    }
    changed = true;
  }
  if (changed && m_anchor >= 0)
  {
    auto moved = this->top(m_anchor) - anchorBefore;
    m_scroll += moved;
    m_target += moved;
  }
  else if (!changed)
    m_anchor = -1;
  m_target = std::clamp(m_target, 0.f, std::max(this->maxScroll(), m_target));
  if (!m_touching || !m_moved)
    m_target = std::min(m_target, std::max(this->maxScroll(), 0.f) + (m_open >= 0 ? m_openFull - m_openExtra : 0.f));

  // Towards the wheel's place, smoothly; a drag moves it at once
  if (m_scroll != m_target)
  {
    auto follow = theme.achievementsScrollSmoothing > 0 ? 1.f - std::exp(-dt / theme.achievementsScrollSmoothing) : 1.f;
    m_scroll += (m_target - m_scroll) * follow;
    if (std::abs(m_target - m_scroll) < 0.05f)
      m_scroll = m_target;
    changed = true;
  }
  m_scroll = std::clamp(m_scroll, 0.f, std::max(this->maxScroll(), 0.f));
  // A page's wave moving the rows: they stay as they are until it's done; gone with the page, they stay gone (laid
  // out again, the rows it hid would show for a frame before the screen is removed)
  if (!m_active || std::any_of(m_rows.begin(), m_rows.end(), [](auto row) { return ms::wave::moving(row); }))
    return;
  this->layout();
  for (auto row : m_rows)
  {
    if (row->isVisible())
      row->step(dt);
  }

  // The row under the mouse lights up, with its shine, once as it comes on
  auto mouse = getMousePos();
  if (m_dragging || (m_touching && m_moved) || !this->canUse())
    return;
  if (mouse.equals(m_lastMouse) && !changed)
    return;
  if (!mouse.equals(m_lastMouse))
    m_cursor = -1;
  else if (m_cursor >= 0)
    return;
  m_lastMouse = mouse;
  this->light(this->rowAt(this->convertToNodeSpace(mouse)), true);
}

CCRect MSAchievementList::trackRect() const
{
  auto box = m_track->boundingBox();
  // Easier to catch than its width
  return {box.origin.x - 4, box.origin.y, box.size.width + 8, box.size.height};
}

bool MSAchievementList::ccTouchBegan(CCTouch* touch, CCEvent* event)
{
  if (!this->canUse())
    return false;
  auto local = this->convertTouchToNodeSpace(touch);
  if (m_track->isVisible() && this->trackRect().containsPoint(local))
  {
    m_dragging = true;
    this->ccTouchMoved(touch, event);
    return true;
  }
  if (!this->viewRect().containsPoint(local))
    return false;
  m_touching = true;
  m_moved = false;
  m_touchStart = local;
  m_scrollStart = m_scroll;
  return true;
}

void MSAchievementList::ccTouchMoved(CCTouch* touch, CCEvent*)
{
  auto local = this->convertTouchToNodeSpace(touch);
  if (m_dragging)
  {
    // The thumb's middle follows the touch
    auto box = m_track->boundingBox();
    auto thumb = m_thumb->getScaledContentSize().height;
    auto travel = std::max(box.size.height - thumb, 1.f);
    auto progress = std::clamp((box.getMaxY() - thumb / 2 - local.y) / travel, 0.f, 1.f);
    m_target = m_scroll = progress * this->maxScroll();
    return;
  }
  if (!m_touching)
    return;
  auto dy = local.y - m_touchStart.y;
  if (!m_moved && std::abs(dy) < DRAG_START)
    return;
  if (!m_moved)
  {
    m_moved = true;
    this->light(-1, false);
  }
  // The content follows the finger: up scrolls on
  m_target = m_scroll = std::clamp(m_scrollStart + dy, 0.f, this->maxScroll());
}

void MSAchievementList::ccTouchEnded(CCTouch* touch, CCEvent*)
{
  if (m_dragging)
  {
    m_dragging = false;
    return;
  }
  if (!m_touching)
    return;
  m_touching = false;
  if (m_moved)
  {
    m_lastMouse = CCPoint{-1, -1};
    return;
  }
  auto index = this->rowAt(this->convertTouchToNodeSpace(touch));
  if (index >= 0)
  {
    m_cursor = -1;
    this->toggle(index);
  }
}

void MSAchievementList::ccTouchCancelled(CCTouch*, CCEvent*)
{
  m_dragging = false;
  m_touching = false;
  m_moved = false;
}
