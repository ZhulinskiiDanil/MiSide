#include "MSPauseColumn.hpp"

#include "../Theme.hpp"
#include "../ui/MSStyle.hpp"

using namespace geode::prelude;

namespace
{
  // A slider's label ends this far before its track; a choice's label leaves this share of the row to its value
  constexpr float TRACK_GAP = 2;
  // A touch on a row that moves this far scrolls the rows (a swipe) instead of pressing it
  constexpr float SWIPE_START = 6;
  constexpr float CHOICE_ROOM = 0.42f;
  constexpr int HIDE_ACTION = 0x4d57;
  // The 9-slice plate's corners are 5.3 pt: a thinner piece is made this much bigger and scaled down, so its
  // corners stay round
  constexpr float SMALL_PANEL = 2;
  // The scroll bar's gap from the rows' right end
  constexpr float BAR_GAP = 4;
  // A field's cursor blinks this often
  constexpr float BLINK_TIME = 0.5f;

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

  // The scroll bar's pieces hang from their top middle
  CCNode* bar(ccColor3B color, float width, float height)
  {
    auto node = ms::createPanel(Theme::get().rowSprite, color, 255);
    node->setAnchorPoint({0.5f, 1});
    node->ignoreAnchorPointForPosition(false);
    node->setContentSize(CCSize{width, height} * SMALL_PANEL);
    node->setScale(1 / SMALL_PANEL);
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

  // The character a key types into a field; 0: none
  char typed(enumKeyCodes key)
  {
    if (key >= KEY_Zero && key <= KEY_Nine)
      return static_cast<char>('0' + (key - KEY_Zero));
    if (key >= KEY_NumPad0 && key <= KEY_NumPad9)
      return static_cast<char>('0' + (key - KEY_NumPad0));
    if (key == KEY_OEMMinus || key == KEY_Subtract)
      return '-';
    if (key == KEY_OEMPeriod || key == KEY_Decimal)
      return '.';
    return 0;
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
  m_rowWidth = Theme::get().pauseRowWidth;
  m_lastMouse = getMousePos();
  // The wheel over a slider
  this->setMouseEnabled(true);
  this->scheduleUpdate();
  return true;
}

void MSPauseColumn::setRowWidth(float width) { m_rowWidth = std::max(width, 40.f); }

void MSPauseColumn::setViewHeight(float height) { m_viewHeight = std::max(height, 0.f); }

float MSPauseColumn::sliderLeft() const
{
  auto const& theme = Theme::get();
  return theme.pauseSliderLeft + (m_rowWidth - theme.pauseRowWidth);
}

MSMenuItem* MSPauseColumn::makeItem(std::string const& text, std::function<void()> action, std::string const& sound,
                                    float reserveRight)
{
  auto style = MSMenuItem::Style::pause();
  if (reserveRight > 0)
  {
    style.fitText = true;
    style.reserveRight = reserveRight;
  }
  auto item = MSMenuItem::create(
    text, nullptr,
    [this, action = std::move(action), sound](MSMenuItem*) {
      if (!this->canUse() || !action)
        return;
      ms::playSound(sound.empty() ? Theme::get().clickSound : sound);
      action();
    },
    m_rowWidth, style);
  if (item)
    this->addChild(item);
  return item;
}

CCLabelBMFont* MSPauseColumn::addValue(MSMenuItem* item, CCNodeRGBA*& control)
{
  auto const& theme = Theme::get();
  control = CCNodeRGBA::create();
  control->setCascadeOpacityEnabled(true);
  control->setAnchorPoint({0, 0});
  CCLabelBMFont* value = nullptr;
  if (auto label = typeinfo_cast<CCLabelBMFont*>(ms::createLabel("-", theme.titleFont)))
  {
    label->setScale(theme.pauseSliderValueScale);
    label->setColor(theme.pauseSliderValueColor);
    // On the row's right, inside its padding
    label->setAnchorPoint({1, 0.5f});
    label->setPosition({m_rowWidth - theme.pauseTextPadding, theme.pauseRowHeight / 2});
    control->addChild(label);
    value = label;
  }
  item->addChild(control, 5);
  return value;
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
  // Its index in m_choices once it's added: the vector may move, the index doesn't
  auto index = m_choices.size();
  auto item = this->makeItem(
    text,
    [this, index] {
      auto& row = m_choices[index];
      if (row.choice.change)
        row.choice.change(1);
      this->showChoice(row);
    },
    "", m_rowWidth * CHOICE_ROOM);
  if (!item)
    return;
  ChoiceRow row;
  row.item = item;
  row.choice = std::move(choice);
  row.value = this->addValue(item, row.control);
  m_choices.push_back(std::move(row));
  this->showChoice(m_choices.back());
  m_rows.push_back(item);
  m_layout.push_back(item);
}

void MSPauseColumn::addField(std::string const& text, Field field)
{
  auto index = m_fields.size();
  auto item = this->makeItem(
    text,
    [this, index] {
      // A click (or Enter) starts typing, another one ends it
      if (m_editing == static_cast<int>(index))
        this->commitEdit();
      else
        this->beginEdit(index);
    },
    "", m_rowWidth * CHOICE_ROOM);
  if (!item)
    return;
  FieldRow row;
  row.item = item;
  row.field = std::move(field);
  row.value = this->addValue(item, row.control);
  m_fields.push_back(std::move(row));
  this->showField(m_fields.back());
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

void MSPauseColumn::setInfo(std::string info)
{
  if (!m_rows.empty())
    m_infos[m_rows.back()] = std::move(info);
}

void MSPauseColumn::setOnInfo(std::function<void(std::string const&)> callback) { m_onInfo = std::move(callback); }

MSPauseColumn::ChoiceRow* MSPauseColumn::choiceFor(MSMenuItem* item)
{
  for (auto& row : m_choices)
  {
    if (row.item == item)
      return &row;
  }
  return nullptr;
}

MSPauseColumn::FieldRow* MSPauseColumn::fieldFor(MSMenuItem* item)
{
  for (auto& row : m_fields)
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

void MSPauseColumn::showField(FieldRow& row)
{
  if (!row.value)
    return;
  std::string text;
  auto editing = m_editing >= 0 && &m_fields[m_editing] == &row;
  if (editing)
  {
    // The typed text, a cursor blinking after it; the value it replaces is shown dimmed until the first key
    auto cursor = std::fmod(m_blink, BLINK_TIME * 2) < BLINK_TIME ? "_" : " ";
    text = m_editText + cursor;
  }
  else if (row.field.get)
    text = row.field.get();
  row.value->setOpacity(editing && m_editFresh ? 160 : 255);
  if (text == row.shown)
    return;
  row.shown = text;
  row.value->setString(text.c_str());
}

void MSPauseColumn::beginEdit(size_t index)
{
  this->commitEdit();
  if (index >= m_fields.size())
    return;
  auto& row = m_fields[index];
  m_editing = static_cast<int>(index);
  m_editText = row.field.get ? row.field.get() : "";
  m_editFresh = true;
  m_blink = 0;
  this->showField(row);
}

void MSPauseColumn::commitEdit()
{
  if (m_editing < 0 || m_editing >= static_cast<int>(m_fields.size()))
  {
    m_editing = -1;
    return;
  }
  auto& row = m_fields[m_editing];
  m_editing = -1;
  // Nothing typed: the value stays
  if (!m_editFresh && !m_editText.empty() && row.field.set)
    row.field.set(m_editText);
  this->showField(row);
}

bool MSPauseColumn::cancelEdit()
{
  if (m_editing < 0)
    return false;
  auto index = m_editing;
  m_editing = -1;
  if (index < static_cast<int>(m_fields.size()))
    this->showField(m_fields[index]);
  return true;
}

std::vector<std::string> MSPauseColumn::rowTexts() const
{
  std::vector<std::string> out;
  for (auto item : m_rows)
    out.push_back(item->getText());
  return out;
}

std::vector<std::string> MSPauseColumn::rowValues()
{
  std::vector<std::string> out;
  for (auto item : m_rows)
  {
    if (auto row = this->choiceFor(item))
      out.push_back(row->choice.value ? row->choice.value() : "");
    else if (auto row = this->fieldFor(item))
      out.push_back(row->field.get ? row->field.get() : "");
    else if (auto row = this->sliderFor(item))
      out.push_back(row->slider.get ? fmt::format("{:.2f}", row->slider.get()) : "");
    else
      out.emplace_back();
  }
  return out;
}

void MSPauseColumn::pressRow(size_t index)
{
  if (index >= m_rows.size())
    return;
  // Scrolled into view first (a row out of view is off), as the keys do
  auto item = std::find(m_items.begin(), m_items.end(), m_rows[index]);
  if (item != m_items.end())
    this->reveal(static_cast<int>(item - m_items.begin()));
  m_rows[index]->activate();
}

bool MSPauseColumn::stepRow(size_t index, int direction)
{
  if (index >= m_rows.size())
    return false;
  auto item = m_rows[index];
  if (auto row = this->choiceFor(item))
  {
    if (!row->choice.change)
      return false;
    row->choice.change(direction);
    this->showChoice(*row);
    return true;
  }
  if (auto row = this->sliderFor(item))
  {
    if (!row->slider.set)
      return false;
    auto step = std::clamp(Theme::get().pauseSliderStep, 0.01f, 0.5f) * direction;
    auto value = std::clamp((row->slider.get ? row->slider.get() : row->shown) + step, 0.f, 1.f);
    row->slider.set(value);
    this->layoutSlider(*row, row->slider.get ? row->slider.get() : value);
    return true;
  }
  return false;
}

std::optional<float> MSPauseColumn::sliderValue(size_t index)
{
  if (index >= m_rows.size())
    return std::nullopt;
  auto row = this->sliderFor(m_rows[index]);
  if (!row || !row->slider.get)
    return std::nullopt;
  return row->slider.get();
}

void MSPauseColumn::setSliderValue(size_t index, float value)
{
  if (index >= m_rows.size())
    return;
  if (auto row = this->sliderFor(m_rows[index]); row && row->slider.set)
  {
    row->slider.set(std::clamp(value, 0.f, 1.f));
    this->layoutSlider(*row, row->slider.get ? row->slider.get() : value);
  }
}

bool MSPauseColumn::onEditKey(enumKeyCodes key)
{
  auto& row = m_fields[m_editing];
  if (key == KEY_Enter || key == CONTROLLER_A)
  {
    ms::playSound(Theme::get().clickSound);
    this->commitEdit();
    return true;
  }
  if (key == KEY_Backspace || key == KEY_Delete)
  {
    if (m_editFresh)
      m_editText.clear();
    else if (!m_editText.empty())
      m_editText.pop_back();
    m_editFresh = false;
    this->showField(row);
    return true;
  }
  auto c = typed(key);
  if (c == 0)
    return false;
  if (row.field.allowed.find(c) == std::string::npos)
    return true;
  if (m_editFresh)
    m_editText.clear();
  m_editFresh = false;
  if (m_editText.size() < row.field.maxLength)
    m_editText.push_back(c);
  this->showField(row);
  return true;
}

void MSPauseColumn::addSlider(std::string const& text, Slider slider)
{
  auto const& theme = Theme::get();
  // Choosing the row itself does nothing: its track is dragged, Left/Right move it. Its label ends before the track
  auto reserve = m_rowWidth - theme.pauseTextPadding - this->sliderLeft() + TRACK_GAP;
  auto item = this->makeItem(text, nullptr, "", reserve);
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
  auto left = this->sliderLeft();
  auto groove = panel(theme.pauseSliderTrackColor, track);
  groove->setPosition({left, middle});
  control->addChild(groove);
  row.fill = panel(theme.pauseSliderFillColor, track);
  row.fill->setPosition({left, middle});
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
    reset->setPosition({left + theme.pauseSliderWidth + 1.5f + size / 2, middle});
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
  m_left = left;
  m_firstTop = firstTop;
  auto slots = static_cast<int>(m_layout.size());
  m_visibleSlots = slots;
  if (m_viewHeight > 0 && theme.pauseRowPitch > 0)
    m_visibleSlots = std::clamp(static_cast<int>(std::floor((m_viewHeight - height) / theme.pauseRowPitch)) + 1, 1,
                                std::max(slots, 1));
  auto scrolls = m_visibleSlots < slots;
  if (m_exit)
  {
    // Under the last row (the view's, when it scrolls) by the usual gap and the theme's own
    auto shown = scrolls ? m_visibleSlots : slots;
    auto lastBottom = firstTop - theme.pauseRowPitch * (static_cast<float>(shown) - 1) - height;
    auto top = lastBottom - (theme.pauseRowPitch - height) - theme.pauseExitGap;
    m_exit->setPosition({left, top - height / 2});
    m_items.push_back(m_exit);
  }
  if (scrolls)
  {
    // A dark track the view's height, a white thumb as tall as the share in view
    auto viewHeight = theme.pauseRowPitch * (m_visibleSlots - 1) + height;
    auto x = left + m_rowWidth + BAR_GAP + theme.screenScrollWidth / 2;
    m_track = bar(theme.achievementsScrollTrackColor, theme.screenScrollWidth, viewHeight);
    m_track->setPosition({x, firstTop});
    this->addChild(m_track, -1);
    auto share = static_cast<float>(m_visibleSlots) / slots;
    m_thumb = bar(theme.achievementsScrollThumbColor, theme.screenScrollWidth, std::max(viewHeight * share, 8.f));
    m_thumb->setPosition({x, firstTop});
    this->addChild(m_thumb, -1);
  }
  this->placeRows();
  for (auto& row : m_sliders)
    this->layoutSlider(row, row.slider.get ? row.slider.get() : 0);
}

// ! --- Scrolling --- !

bool MSPauseColumn::inView(CCNode* node) const
{
  auto it = std::find(m_layout.begin(), m_layout.end(), node);
  if (it == m_layout.end())
    return true;
  auto slot = static_cast<int>(it - m_layout.begin()) - m_offset;
  return slot >= 0 && slot < m_visibleSlots;
}

void MSPauseColumn::placeRows()
{
  auto const& theme = Theme::get();
  auto height = theme.pauseRowHeight;
  for (size_t i = 0; i < m_layout.size(); i++)
  {
    auto node = m_layout[i];
    auto slot = static_cast<int>(i) - m_offset;
    auto middle = m_firstTop - theme.pauseRowPitch * slot - height / 2;
    // A group's name sits in its place by the rows' text, a little lower than a row's middle
    if (std::find(m_headers.begin(), m_headers.end(), node) != m_headers.end())
      node->setPosition({m_left + 2, middle - height * 0.15f});
    else
      node->setPosition({m_left, middle});
    auto shown = slot >= 0 && slot < m_visibleSlots;
    node->setVisible(shown);
    if (auto item = typeinfo_cast<MSMenuItem*>(node))
      item->setEnabled(shown && m_isShown);
  }
  if (m_thumb && m_track)
  {
    auto slots = static_cast<int>(m_layout.size());
    auto travel = std::max(m_track->getScaledContentSize().height - m_thumb->getScaledContentSize().height, 0.f);
    auto progress = slots > m_visibleSlots ? static_cast<float>(m_offset) / (slots - m_visibleSlots) : 0.f;
    m_thumb->setPositionY(m_firstTop - travel * progress);
  }
}

void MSPauseColumn::scrollTo(int offset)
{
  auto most = std::max(0, static_cast<int>(m_layout.size()) - m_visibleSlots);
  offset = std::clamp(offset, 0, most);
  if (offset == m_offset)
    return;
  m_offset = offset;
  this->placeRows();
  // The mouse is over another row now
  m_lastMouse = CCPoint{-1, -1};
  if (m_shown >= 0 && m_shown < static_cast<int>(m_items.size()) && !this->inView(m_items[m_shown]))
    this->highlight(-1, false);
}

void MSPauseColumn::reveal(int index)
{
  if (index < 0 || index >= static_cast<int>(m_items.size()))
    return;
  auto it = std::find(m_layout.begin(), m_layout.end(), m_items[index]);
  if (it == m_layout.end())
    return;
  auto slot = static_cast<int>(it - m_layout.begin());
  if (slot < m_offset)
  {
    // Its group's name right over it comes into view with it
    auto header = slot > 0 && std::find(m_headers.begin(), m_headers.end(), m_layout[slot - 1]) != m_headers.end();
    this->scrollTo(header ? slot - 1 : slot);
  }
  else if (slot >= m_offset + m_visibleSlots)
    this->scrollTo(slot - m_visibleSlots + 1);
}

void MSPauseColumn::layoutSlider(SliderRow& row, float value)
{
  auto const& theme = Theme::get();
  value = std::clamp(value, 0.f, 1.f);
  row.shown = value;
  auto knobWidth = std::min(theme.pauseSliderKnobWidth, theme.pauseSliderWidth);
  auto travel = theme.pauseSliderWidth - knobWidth;
  auto left = this->sliderLeft();
  auto knobLeft = left + travel * value;
  auto middle = theme.pauseRowHeight / 2;
  row.knob->setPosition({knobLeft, middle});
  // The fill to the knob's right edge
  row.fill->setContentSize(CCSize{knobLeft + knobWidth - left, theme.pauseSliderHeight} * SMALL_PANEL);
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
  return {origin.x + this->sliderLeft() - 2, origin.y, theme.pauseSliderWidth + 4, theme.pauseRowHeight};
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
  auto value = std::clamp((local.x - origin - this->sliderLeft() - knobWidth / 2) / travel, 0.f, 1.f);
  if (row.slider.set)
    row.slider.set(value);
  this->layoutSlider(row, row.slider.get ? row.slider.get() : value);
}

void MSPauseColumn::setShown(bool shown, bool animate)
{
  if (shown == m_isShown)
    return;
  this->commitEdit();
  auto const& theme = Theme::get();
  // Its rows as they are now, before any of them is taken out of the keys' reach
  auto steps = this->pageSteps();
  m_isShown = shown;
  m_dragging = nullptr;
  m_resetting = nullptr;
  m_tracking = m_swiping = false;
  if (!shown)
    m_restore = m_shown;
  this->highlight(-1, false);
  this->stopActionByTag(HIDE_ACTION);
  // Nothing is chosen from the first frame of going; each one back at rest, its shine stopped
  for (auto item : m_items)
  {
    item->setEnabled(shown && this->inView(item));
    item->resetLook();
  }
  if (shown)
  {
    this->setVisible(true);
    m_lastMouse = CCPoint{-1, -1};
    // Where the keys (or the mouse) were when it hid
    if (m_restore >= 0 && m_restore < static_cast<int>(m_items.size()))
    {
      this->reveal(m_restore);
      this->highlight(m_restore, false);
    }
    m_restore = -1;
    // From the left, as a page comes (the one going leaves first)
    if (animate)
      ms::wave::in(this->pageSteps(), theme.pageInDelay);
    else
      ms::wave::show(this->pageSteps());
    return;
  }
  if (!animate)
  {
    ms::wave::hide(steps);
    this->setVisible(false);
    return;
  }
  // To the right, as a page goes; hidden as a whole once it has
  auto gone = ms::wave::out(steps);
  auto hide = CCSequence::createWithTwoActions(CCDelayTime::create(gone), CCHide::create());
  hide->setTag(HIDE_ACTION);
  this->runAction(hide);
}

bool MSPauseColumn::isShown() const { return m_isShown; }

ms::wave::Steps MSPauseColumn::pageSteps()
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
  for (auto node : m_layout)
  {
    if (ms::wave::shows(node))
      steps.push_back({{node}});
  }
  if (ms::wave::shows(m_exit))
    steps.push_back({{m_exit}});
  return steps;
}

bool MSPauseColumn::canUse() { return m_isShown && this->isVisible() && !ms::coveredBy(this); }

void MSPauseColumn::highlight(int index, bool shine)
{
  if (index == m_shown)
    return;
  // Another row chosen: the field typed in is done
  if (m_editing >= 0 && (index < 0 || index >= static_cast<int>(m_items.size()) ||
                         m_items[index] != m_fields[m_editing].item))
    this->commitEdit();
  if (m_shown >= 0 && m_shown < static_cast<int>(m_items.size()))
    m_items[m_shown]->setHighlighted(false);
  m_shown = index;
  if (m_shown >= 0)
    m_items[m_shown]->setHighlighted(true, shine);
  if (m_onInfo)
  {
    std::string info;
    if (m_shown >= 0)
    {
      if (auto it = m_infos.find(m_items[m_shown]); it != m_infos.end())
        info = it->second;
    }
    m_onInfo(info);
  }
}

bool MSPauseColumn::onKey(enumKeyCodes key)
{
  if (m_items.empty() || !this->canUse())
    return false;
  if (m_editing >= 0 && this->onEditKey(key))
    return true;
  auto count = static_cast<int>(m_items.size());
  if (isUp(key) || isDown(key))
  {
    auto next = m_shown < 0 ? (isUp(key) ? count - 1 : 0) : (m_shown + (isUp(key) ? count - 1 : 1)) % count;
    this->reveal(next);
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
    if (auto field = this->fieldFor(item))
    {
      if (!field->field.step || m_editing >= 0)
        return false;
      ms::playSound(Theme::get().clickSound);
      field->field.step(isLeft(key) ? -1 : 1);
      this->showField(*field);
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
  m_blink += dt;
  // A change made elsewhere (GD's settings over the pause) shows on the sliders, the choices and the fields
  for (auto& row : m_choices)
    this->showChoice(row);
  for (auto& row : m_fields)
    this->showField(row);
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
    if (m_items[i]->isVisible() && m_items[i]->boundingBox().containsPoint(local))
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
  // One row a notch, the wheel's way
  if (m_visibleSlots < static_cast<int>(m_layout.size()))
    this->scrollTo(m_offset + (y > 0 ? 1 : -1));
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
  if (!CCMenu::ccTouchBegan(touch, event))
    return false;
  m_tracking = true;
  m_swiping = false;
  m_swipeFrom = touch->getLocation().y;
  m_swipeOffset = m_offset;
  return true;
}

void MSPauseColumn::ccTouchMoved(CCTouch* touch, CCEvent* event)
{
  if (m_dragging)
    return this->dragTo(*m_dragging, this->convertTouchToNodeSpace(touch));
  if (m_resetting)
    return;
  // Only a column longer than its view scrolls; up shows the rows under, a slot at a time
  if (m_tracking && m_visibleSlots < static_cast<int>(m_layout.size()))
  {
    auto moved = touch->getLocation().y - m_swipeFrom;
    if (!m_swiping && std::abs(moved) >= SWIPE_START)
    {
      m_swiping = true;
      CCMenu::ccTouchCancelled(touch, event);
    }
    if (m_swiping)
    {
      auto pitch = std::max(Theme::get().pauseRowPitch, 1.f);
      this->scrollTo(m_swipeOffset + static_cast<int>(std::lround(moved / pitch)));
      return;
    }
  }
  CCMenu::ccTouchMoved(touch, event);
}

void MSPauseColumn::ccTouchEnded(CCTouch* touch, CCEvent* event)
{
  auto swiped = m_swiping;
  m_tracking = m_swiping = false;
  if (swiped)
    return;
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
  auto swiped = m_swiping;
  m_tracking = m_swiping = false;
  if (swiped)
    return;
  if (m_dragging || m_resetting)
  {
    m_dragging = nullptr;
    m_resetting = nullptr;
    return;
  }
  CCMenu::ccTouchCancelled(touch, event);
}
