#include "MSPauseColumn.hpp"

#include "../Theme.hpp"
#include "../ui/MSStyle.hpp"

using namespace geode::prelude;

namespace
{
  constexpr int HIDE_ACTION = 0x4d57;
  constexpr int CONTROL_ACTION = 0x4d58;
  // The 9-slice plate's corners are 5.3 pt: a thinner piece is made this much bigger and scaled down, so its
  // corners stay round
  constexpr float SMALL_PANEL = 2;

  CCNode* panel(ccColor3B color, CCSize size)
  {
    auto node = ms::createPanel(Theme::get().rowSprite, color, 255);
    node->setAnchorPoint({0, 0.5f});
    node->ignoreAnchorPointForPosition(false);
    node->setContentSize(size * SMALL_PANEL);
    node->setScale(1 / SMALL_PANEL);
    ms::cascadeOpacity(node);
    return node;
  }

  bool isUp(enumKeyCodes key) { return key == KEY_Up || key == CONTROLLER_Up || key == CONTROLLER_LTHUMBSTICK_UP; }
  bool isDown(enumKeyCodes key)
  {
    return key == KEY_Down || key == CONTROLLER_Down || key == CONTROLLER_LTHUMBSTICK_DOWN;
  }
  bool isLeft(enumKeyCodes key)
  {
    return key == KEY_Left || key == CONTROLLER_Left || key == CONTROLLER_LTHUMBSTICK_LEFT;
  }
  bool isRight(enumKeyCodes key)
  {
    return key == KEY_Right || key == CONTROLLER_Right || key == CONTROLLER_LTHUMBSTICK_RIGHT;
  }
} // namespace

MSPauseColumn* MSPauseColumn::create()
{
  auto column = new MSPauseColumn();
  if (column->init())
  {
    column->autorelease();
    return column;
  }
  delete column;
  return nullptr;
}

bool MSPauseColumn::init()
{
  if (!CCMenu::init())
    return false;
  // Everything is placed in the layer's own points
  this->setPosition({0, 0});
  this->setContentSize({0, 0});
  m_lastMouse = getMousePos();
  // The wheel over a slider
  this->setMouseEnabled(true);
  this->scheduleUpdate();
  return true;
}

MSMenuItem* MSPauseColumn::makeItem(std::string const& text, std::function<void()> action, std::string const& sound)
{
  auto const& theme = Theme::get();
  auto item = MSMenuItem::create(
    text, nullptr,
    [this, action = std::move(action), sound](MSMenuItem*) {
      if (!this->canUse() || !action)
        return;
      ms::playSound(sound.empty() ? Theme::get().clickSound : sound);
      action();
    },
    theme.pauseRowWidth, MSMenuItem::Style::pause());
  if (item)
    this->addChild(item);
  return item;
}

void MSPauseColumn::addRow(std::string const& text, std::function<void()> action, std::string const& sound)
{
  if (auto item = this->makeItem(text, std::move(action), sound))
  {
    m_rows.push_back(item);
    m_layout.push_back(item);
  }
}

void MSPauseColumn::addChoice(std::string const& text, Choice choice)
{
  auto const& theme = Theme::get();
  // Its index in m_choices once it's added: the vector may move, the index doesn't
  auto index = m_choices.size();
  auto item = this->makeItem(text, [this, index] {
    auto& row = m_choices[index];
    if (row.choice.change)
      row.choice.change(1);
    this->showChoice(row);
  });
  if (!item)
    return;
  ChoiceRow row;
  row.item = item;
  row.choice = std::move(choice);
  auto control = CCNodeRGBA::create();
  control->setCascadeOpacityEnabled(true);
  control->setAnchorPoint({0, 0});
  if (auto label = typeinfo_cast<CCLabelBMFont*>(ms::createLabel("-", theme.titleFont)))
  {
    label->setScale(theme.pauseSliderValueScale);
    label->setColor(theme.pauseSliderValueColor);
    // On the row's right, inside its padding
    label->setAnchorPoint({1, 0.5f});
    label->setPosition({theme.pauseRowWidth - theme.pauseTextPadding, theme.pauseRowHeight / 2});
    control->addChild(label);
    row.value = label;
  }
  item->addChild(control, 5);
  row.control = control;
  m_choices.push_back(std::move(row));
  this->showChoice(m_choices.back());
  m_rows.push_back(item);
  m_layout.push_back(item);
}

void MSPauseColumn::addHeader(std::string const& text)
{
  auto const& theme = Theme::get();
  auto header = ms::createText(text, theme.statsGroupScale, theme.statsGroupColor, theme.titleFont, ms::Shadow{});
  if (!header.node)
    return;
  this->addChild(header.node);
  m_headers.push_back(header.node);
  m_layout.push_back(header.node);
}

MSPauseColumn::ChoiceRow* MSPauseColumn::choiceFor(MSMenuItem* item)
{
  for (auto& row : m_choices)
  {
    if (row.item == item)
      return &row;
  }
  return nullptr;
}

void MSPauseColumn::showChoice(ChoiceRow& row)
{
  if (!row.value || !row.choice.value)
    return;
  auto text = row.choice.value();
  if (text == row.shown)
    return;
  row.shown = text;
  row.value->setString(text.c_str());
}

void MSPauseColumn::addSlider(std::string const& text, Slider slider)
{
  auto const& theme = Theme::get();
  // Choosing the row itself does nothing: its track is dragged, Left/Right move it
  auto item = this->makeItem(text, nullptr);
  if (!item)
    return;
  SliderRow row;
  row.item = item;
  row.slider = std::move(slider);

  auto control = CCNodeRGBA::create();
  control->setCascadeOpacityEnabled(true);
  control->setAnchorPoint({0, 0});
  CCSize track{theme.pauseSliderWidth, theme.pauseSliderHeight};
  auto middle = theme.pauseRowHeight / 2;
  auto groove = panel(theme.pauseSliderTrackColor, track);
  groove->setPosition({theme.pauseSliderLeft, middle});
  control->addChild(groove);
  row.fill = panel(theme.pauseSliderFillColor, track);
  row.fill->setPosition({theme.pauseSliderLeft, middle});
  control->addChild(row.fill);
  row.knob = panel(theme.pauseSliderKnobColor, {theme.pauseSliderKnobWidth, theme.pauseSliderHeight});
  control->addChild(row.knob);
  if (auto label = typeinfo_cast<CCLabelBMFont*>(ms::createLabel("100", theme.titleFont)))
  {
    label->setScale(theme.pauseSliderValueScale);
    label->setColor(theme.pauseSliderValueColor);
    label->setAnchorPoint({0.5f, 0.5f});
    control->addChild(label);
    row.value = label;
  }
  if (row.slider.reset >= 0)
  {
    // A pink circle with a white ring and an arrow going round, after the track
    auto size = theme.pauseResetSize;
    auto reset = CCNodeRGBA::create();
    reset->setCascadeOpacityEnabled(true);
    reset->setContentSize({size, size});
    reset->setAnchorPoint({0.5f, 0.5f});
    reset->setPosition({theme.pauseSliderLeft + theme.pauseSliderWidth + 1.5f + size / 2, middle});
    if (auto circle = ms::createStretched(theme.avatarSprite, {size, size}, theme.pauseResetColor, 255))
    {
      circle->setAnchorPoint({0.5f, 0.5f});
      circle->setPosition({size / 2, size / 2});
      reset->addChild(circle);
    }
    auto arrow = CCDrawNode::create();
    std::vector<CCPoint> arc;
    auto radius = size * 0.27f;
    for (int i = 0; i <= 12; i++)
    {
      auto angle = 0.6f + i * (4.6f / 12);
      arc.emplace_back(size / 2 + std::cos(angle) * radius, size / 2 + std::sin(angle) * radius);
    }
    for (size_t i = 1; i < arc.size(); i++)
      arrow->drawSegment(arc[i - 1], arc[i], size * 0.06f, ccc4f(1, 1, 1, 1));
    auto tip = arc.front();
    CCPoint head[] = {{tip.x + size * 0.16f, tip.y + size * 0.02f}, {tip.x - size * 0.02f, tip.y + size * 0.17f},
                      {tip.x - size * 0.05f, tip.y - size * 0.06f}};
    arrow->drawPolygon(head, 3, ccc4f(1, 1, 1, 1), 0, ccc4f(1, 1, 1, 1));
    reset->addChild(arrow);
    control->addChild(reset);
    row.reset = reset;
  }
  item->addChild(control, 5);
  row.control = control;
  m_rows.push_back(item);
  m_layout.push_back(item);
  m_sliders.push_back(std::move(row));
}

void MSPauseColumn::setExit(std::string const& text, std::function<void()> action, std::string const& sound)
{
  m_exit = this->makeItem(text, std::move(action), sound);
}

void MSPauseColumn::finish(float left, float firstTop)
{
  auto const& theme = Theme::get();
  auto height = theme.pauseRowHeight;
  m_items = m_rows;
  // A group's name sits in its place by the rows' text, a little lower than a row's middle
  for (size_t i = 0; i < m_layout.size(); i++)
  {
    auto middle = firstTop - theme.pauseRowPitch * i - height / 2;
    auto node = m_layout[i];
    if (std::find(m_headers.begin(), m_headers.end(), node) != m_headers.end())
      node->setPosition({left + 2, middle - height * 0.15f});
    else
      node->setPosition({left, middle});
  }
  if (m_exit)
  {
    // Under the last row by the usual gap and the theme's own
    auto lastBottom = firstTop - theme.pauseRowPitch * (static_cast<float>(m_layout.size()) - 1) - height;
    auto top = lastBottom - (theme.pauseRowPitch - height) - theme.pauseExitGap;
    m_exit->setPosition({left, top - height / 2});
    m_items.push_back(m_exit);
  }
  for (auto& row : m_sliders)
    this->layoutSlider(row, row.slider.get ? row.slider.get() : 0);
}

void MSPauseColumn::layoutSlider(SliderRow& row, float value)
{
  auto const& theme = Theme::get();
  value = std::clamp(value, 0.f, 1.f);
  row.shown = value;
  auto knobWidth = std::min(theme.pauseSliderKnobWidth, theme.pauseSliderWidth);
  auto travel = theme.pauseSliderWidth - knobWidth;
  auto knobLeft = theme.pauseSliderLeft + travel * value;
  auto middle = theme.pauseRowHeight / 2;
  row.knob->setPosition({knobLeft, middle});
  // The fill to the knob's right edge
  row.fill->setContentSize(CCSize{knobLeft + knobWidth - theme.pauseSliderLeft, theme.pauseSliderHeight} * SMALL_PANEL);
  if (row.value)
  {
    auto text = row.slider.format ? row.slider.format(value) : fmt::format("{}", static_cast<int>(std::round(value * 100)));
    row.value->setString(text.c_str());
    row.value->setPosition({knobLeft + knobWidth / 2, middle});
  }
}

CCRect MSPauseColumn::trackRect(SliderRow const& row) const
{
  auto const& theme = Theme::get();
  auto origin = row.item->getPosition() - CCPoint{0, theme.pauseRowHeight / 2};
  // The whole row's height, easier to catch
  return {origin.x + theme.pauseSliderLeft - 2, origin.y, theme.pauseSliderWidth + 4, theme.pauseRowHeight};
}

CCRect MSPauseColumn::resetRect(SliderRow const& row) const
{
  if (!row.reset)
    return {};
  auto box = row.reset->boundingBox();
  auto origin = row.item->getPosition() - CCPoint{0, Theme::get().pauseRowHeight / 2};
  return {origin.x + box.origin.x, origin.y + box.origin.y, box.size.width, box.size.height};
}

MSPauseColumn::SliderRow* MSPauseColumn::sliderFor(MSMenuItem* item)
{
  for (auto& row : m_sliders)
  {
    if (row.item == item)
      return &row;
  }
  return nullptr;
}

void MSPauseColumn::dragTo(SliderRow& row, CCPoint local)
{
  auto const& theme = Theme::get();
  auto origin = row.item->getPositionX();
  auto knobWidth = std::min(theme.pauseSliderKnobWidth, theme.pauseSliderWidth);
  auto travel = std::max(theme.pauseSliderWidth - knobWidth, 1.f);
  auto value = std::clamp((local.x - origin - theme.pauseSliderLeft - knobWidth / 2) / travel, 0.f, 1.f);
  if (row.slider.set)
    row.slider.set(value);
  this->layoutSlider(row, row.slider.get ? row.slider.get() : value);
}

void MSPauseColumn::setShown(bool shown, bool animate)
{
  if (shown == m_isShown)
    return;
  m_isShown = shown;
  auto time = animate ? std::max(Theme::get().pauseDuration, 0.f) : 0.f;
  m_dragging = nullptr;
  m_resetting = nullptr;
  if (!shown)
    m_restore = m_shown;
  m_shown = -1;
  this->stopActionByTag(HIDE_ACTION);
  for (auto item : m_items)
  {
    // Nothing is chosen from the first frame of fading out; each one back at rest, its shine stopped
    item->setEnabled(shown);
    item->resetLook();
    item->fadeTo(time, shown ? 255 : 0);
  }
  // The sliders' and choices' controls and the groups' names fade with the rows
  std::vector<CCNode*> extras(m_headers.begin(), m_headers.end());
  for (auto& row : m_sliders)
    extras.push_back(row.control);
  for (auto& row : m_choices)
    extras.push_back(row.control);
  for (auto node : extras)
  {
    node->stopActionByTag(CONTROL_ACTION);
    if (time <= 0)
    {
      ms::setOpacity(node, shown ? 255 : 0);
      continue;
    }
    auto fade = CCFadeTo::create(time, shown ? 255 : 0);
    fade->setTag(CONTROL_ACTION);
    node->runAction(fade);
  }
  if (shown)
  {
    this->setVisible(true);
    m_lastMouse = CCPoint{-1, -1};
    // Where the keys (or the mouse) were when it hid
    if (m_restore >= 0 && m_restore < static_cast<int>(m_items.size()))
      this->highlight(m_restore, false);
    m_restore = -1;
    return;
  }
  if (time <= 0)
  {
    this->setVisible(false);
    return;
  }
  auto hide = CCSequence::createWithTwoActions(CCDelayTime::create(time), CCHide::create());
  hide->setTag(HIDE_ACTION);
  this->runAction(hide);
}

bool MSPauseColumn::isShown() const { return m_isShown; }

bool MSPauseColumn::canUse() { return m_isShown && this->isVisible() && !ms::coveredBy(this); }

void MSPauseColumn::highlight(int index, bool shine)
{
  if (index == m_shown)
    return;
  if (m_shown >= 0 && m_shown < static_cast<int>(m_items.size()))
    m_items[m_shown]->setHighlighted(false);
  m_shown = index;
  if (m_shown >= 0)
    m_items[m_shown]->setHighlighted(true, shine);
}

bool MSPauseColumn::onKey(enumKeyCodes key)
{
  if (m_items.empty() || !this->canUse())
    return false;
  auto count = static_cast<int>(m_items.size());
  if (isUp(key) || isDown(key))
  {
    auto next = m_shown < 0 ? (isUp(key) ? count - 1 : 0) : (m_shown + (isUp(key) ? count - 1 : 1)) % count;
    this->highlight(next, true);
    ms::playSound(Theme::get().hoverSound);
    return true;
  }
  if (m_shown < 0)
    return false;
  auto item = m_items[m_shown];
  if (isLeft(key) || isRight(key))
  {
    if (auto choice = this->choiceFor(item))
    {
      if (!choice->choice.change)
        return false;
      ms::playSound(Theme::get().clickSound);
      choice->choice.change(isLeft(key) ? -1 : 1);
      this->showChoice(*choice);
      return true;
    }
    auto row = this->sliderFor(item);
    if (!row || !row->slider.set)
      return false;
    auto step = std::clamp(Theme::get().pauseSliderStep, 0.01f, 0.5f) * (isLeft(key) ? -1 : 1);
    auto value = std::clamp((row->slider.get ? row->slider.get() : row->shown) + step, 0.f, 1.f);
    row->slider.set(value);
    this->layoutSlider(*row, row->slider.get ? row->slider.get() : value);
    return true;
  }
  if (key == KEY_Enter || key == KEY_Space || key == CONTROLLER_A)
  {
    item->activate();
    return true;
  }
  return false;
}

void MSPauseColumn::update(float dt)
{
  // A change made elsewhere (GD's settings over the pause) shows on the sliders and the choices
  for (auto& row : m_choices)
    this->showChoice(row);
  for (auto& row : m_sliders)
  {
    if (!row.slider.get || row.item == nullptr || (m_dragging == &row))
      continue;
    auto value = std::clamp(row.slider.get(), 0.f, 1.f);
    if (std::abs(value - row.shown) > 0.001f)
      this->layoutSlider(row, value);
  }
  auto mouse = getMousePos();
  if (mouse.equals(m_lastMouse) || m_dragging || !this->canUse())
    return;
  m_lastMouse = mouse;
  // The row under the mouse lights up; off the rows, the last one stays chosen for the keys
  auto local = this->convertToNodeSpace(mouse);
  for (int i = 0; i < static_cast<int>(m_items.size()); i++)
  {
    if (m_items[i]->boundingBox().containsPoint(local))
    {
      if (i != m_shown)
      {
        this->highlight(i, true);
        ms::playSound(Theme::get().hoverSound);
      }
      return;
    }
  }
}

void MSPauseColumn::scrollWheel(float y, float)
{
  if (y == 0 || !this->canUse())
    return;
  auto local = this->convertToNodeSpace(getMousePos());
  for (auto& row : m_sliders)
  {
    if (!row.slider.set || !row.item->isVisible() || !row.item->boundingBox().containsPoint(local))
      continue;
    // The wheel down lowers it
    auto step = std::clamp(Theme::get().pauseSliderStep, 0.01f, 0.5f) * (y > 0 ? -1 : 1);
    auto value = std::clamp((row.slider.get ? row.slider.get() : row.shown) + step, 0.f, 1.f);
    row.slider.set(value);
    this->layoutSlider(row, row.slider.get ? row.slider.get() : value);
    return;
  }
}

bool MSPauseColumn::ccTouchBegan(CCTouch* touch, CCEvent* event)
{
  if (!this->canUse())
    return false;
  auto local = this->convertTouchToNodeSpace(touch);
  for (auto& row : m_sliders)
  {
    if (!row.item->isVisible())
      continue;
    if (row.reset && this->resetRect(row).containsPoint(local))
    {
      m_resetting = &row;
      return true;
    }
    if (this->trackRect(row).containsPoint(local))
    {
      m_dragging = &row;
      this->highlight(static_cast<int>(std::find(m_items.begin(), m_items.end(), row.item) - m_items.begin()), false);
      this->dragTo(row, local);
      return true;
    }
  }
  return CCMenu::ccTouchBegan(touch, event);
}

void MSPauseColumn::ccTouchMoved(CCTouch* touch, CCEvent* event)
{
  if (m_dragging)
    return this->dragTo(*m_dragging, this->convertTouchToNodeSpace(touch));
  if (m_resetting)
    return;
  CCMenu::ccTouchMoved(touch, event);
}

void MSPauseColumn::ccTouchEnded(CCTouch* touch, CCEvent* event)
{
  if (m_dragging)
  {
    m_dragging = nullptr;
    return;
  }
  if (auto row = m_resetting)
  {
    m_resetting = nullptr;
    if (this->resetRect(*row).containsPoint(this->convertTouchToNodeSpace(touch)) && row->slider.set)
    {
      ms::playSound(Theme::get().clickSound);
      row->slider.set(std::clamp(row->slider.reset, 0.f, 1.f));
      this->layoutSlider(*row, row->slider.get ? row->slider.get() : row->slider.reset);
    }
    return;
  }
  CCMenu::ccTouchEnded(touch, event);
}

void MSPauseColumn::ccTouchCancelled(CCTouch* touch, CCEvent* event)
{
  if (m_dragging || m_resetting)
  {
    m_dragging = nullptr;
    m_resetting = nullptr;
    return;
  }
  CCMenu::ccTouchCancelled(touch, event);
}
