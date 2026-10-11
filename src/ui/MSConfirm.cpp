#include "MSConfirm.hpp"

#include "../Theme.hpp"
#include "MSStyle.hpp"

using namespace geode::prelude;

namespace
{
  constexpr int FADE_ACTION = 0x4d55;
  constexpr int YES = 0;
  constexpr int NO = 1;
} // namespace

MSConfirm::Layout MSConfirm::Layout::menu()
{
  auto const& theme = Theme::get();
  return {theme.confirmTitleLeft, theme.confirmTitleCapTop, theme.confirmTitleScale,   theme.confirmRowLeft,
          theme.confirmFirstRowTop, theme.confirmRowPitch, theme.confirmRowWidth};
}

MSConfirm* MSConfirm::create(Texts const& texts, std::function<void()> yes, std::function<void()> cancel,
                             Layout const& layout, MSMenuItem::Style const& style)
{
  auto confirm = new MSConfirm();
  if (confirm->init(texts, std::move(yes), std::move(cancel), layout, style))
  {
    confirm->autorelease();
    return confirm;
  }
  delete confirm;
  return nullptr;
}

bool MSConfirm::init(Texts const& texts, std::function<void()> yes, std::function<void()> cancel, Layout const& layout,
                     MSMenuItem::Style const& style)
{
  if (!CCMenu::init())
    return false;
  auto const& theme = Theme::get();
  m_yes = std::move(yes);
  m_cancel = std::move(cancel);
  // Everything is placed in the layer's own points
  this->setPosition({0, 0});
  this->setContentSize({0, 0});

  // The title: white, a solid dark shadow; the top of its capitals where the theme says. From the title font's
  // metrics at scale 1: capitals 17.5 pt high, their middle 2.82 pt over the label's middle
  ms::Shadow shadow{theme.confirmTitleShadowColor, theme.confirmTitleShadowOpacity, 1, theme.confirmTitleShadowOffset};
  auto title = ms::createText(texts.title, layout.titleScale, theme.confirmTitleColor, theme.titleFont, shadow);
  if (!title.node)
  {
    log::error("The quit confirmation's title can't be made: the vanilla popup instead");
    return false;
  }
  m_titleText = title.node;
  constexpr float TITLE_CAP = 17.5f;
  constexpr float TITLE_CAP_RAISE = 2.82f;
  auto middle = layout.titleCapTop - (TITLE_CAP / 2 + TITLE_CAP_RAISE) * layout.titleScale;
  auto holder = CCMenuItem::create();
  holder->setEnabled(false);
  holder->setContentSize(m_titleText->getContentSize());
  holder->setAnchorPoint({0, 0.5f});
  holder->setPosition({layout.titleLeft, middle});
  m_titleText->setAnchorPoint({0, 0});
  m_titleText->ignoreAnchorPointForPosition(false);
  m_titleText->setPosition({0, 0});
  holder->addChild(m_titleText);
  this->addChild(holder);
  m_title = holder;

  for (auto const& text : {texts.yes, texts.no})
  {
    auto item = MSMenuItem::create(text, nullptr, [this](MSMenuItem* item) { this->choose(item); }, layout.rowWidth,
                                   style);
    if (!item)
      return false;
    auto index = static_cast<float>(m_items.size());
    // Anchored at its left middle
    item->setPosition({layout.rowLeft, layout.firstRowTop - layout.rowPitch * index - style.height / 2});
    this->addChild(item);
    m_items.push_back(item);
  }

  this->setShown(0, 0);
  this->setVisible(false);
  this->scheduleUpdate();
  return true;
}

void MSConfirm::setShown(float opacity, float time)
{
  auto value = static_cast<GLubyte>(opacity * 255);
  for (auto item : m_items)
    item->fadeTo(time, value);
  m_titleText->stopActionByTag(FADE_ACTION);
  if (time <= 0)
  {
    ms::setOpacity(m_titleText, value);
    return;
  }
  auto fade = CCFadeTo::create(time, value);
  fade->setTag(FADE_ACTION);
  m_titleText->runAction(fade);
}

void MSConfirm::open()
{
  auto const& theme = Theme::get();
  this->stopActionByTag(FADE_ACTION);
  m_open = true;
  m_done = false;
  m_age = 0;
  m_lastMouse = getMousePos();
  for (auto item : m_items)
  {
    item->setEnabled(true);
    item->setHighlighted(false);
  }
  m_shown = -1;
  this->setVisible(true);
  this->setShown(1, theme.confirmDuration);
  // No first, so a stray Enter or click can't quit
  this->highlight(NO, true);
}

void MSConfirm::close()
{
  auto const& theme = Theme::get();
  m_open = false;
  // Nothing is chosen from now, though it's still fading out
  for (auto item : m_items)
    item->setEnabled(false);
  this->highlight(-1, false);
  this->setShown(0, theme.confirmDuration);
  this->stopActionByTag(FADE_ACTION);
  auto hide = CCSequence::createWithTwoActions(CCDelayTime::create(std::max(theme.confirmDuration, 0.f)),
                                                CCHide::create());
  hide->setTag(FADE_ACTION);
  this->runAction(hide);
}

bool MSConfirm::isOpen() const { return m_open; }

bool MSConfirm::canChoose()
{
  if (!m_open || m_done || m_age < Theme::get().confirmDuration)
    return false;
  return !ms::coveredBy(this);
}

void MSConfirm::highlight(int index, bool shine)
{
  if (index == m_shown)
    return;
  if (m_shown >= 0)
    m_items[m_shown]->setHighlighted(false);
  m_shown = index;
  if (m_shown >= 0)
    m_items[m_shown]->setHighlighted(true, shine);
}

void MSConfirm::onKey(enumKeyCodes key)
{
  if (!m_open || m_done)
    return;
  switch (key)
  {
  case KEY_Up:
  case KEY_Down:
  case CONTROLLER_Up:
  case CONTROLLER_Down:
  case CONTROLLER_LTHUMBSTICK_UP:
  case CONTROLLER_LTHUMBSTICK_DOWN:
  case KEY_Tab:
    // Two buttons: either key goes to the other one
    this->highlight(m_shown == YES ? NO : YES, true);
    ms::playSound(Theme::get().hoverSound);
    return;
  case KEY_Enter:
  case KEY_Space:
  case CONTROLLER_A:
  case CONTROLLER_Start:
    if (m_shown >= 0 && this->canChoose())
      m_items[m_shown]->activate();
    return;
  case KEY_Escape:
  case CONTROLLER_B:
  case CONTROLLER_Back:
    this->keyBack();
    return;
  default:
    return;
  }
}

void MSConfirm::keyBack()
{
  if (!m_open || m_done)
    return;
  ms::playSound(Theme::get().clickSound);
  if (m_cancel)
    m_cancel();
}

void MSConfirm::choose(MSMenuItem* item)
{
  if (!this->canChoose())
    return;
  ms::playSound(Theme::get().clickSound);
  if (item == m_items[YES])
  {
    // Once: whatever comes next (a second click, Enter held) finds it done
    m_done = true;
    for (auto each : m_items)
      each->setEnabled(false);
    if (m_yes)
      m_yes();
  }
  else if (m_cancel)
    m_cancel();
}

void MSConfirm::update(float dt)
{
  if (!m_open)
    return;
  m_age += dt;
  auto mouse = getMousePos();
  if (mouse.equals(m_lastMouse) || m_done || ms::coveredBy(this))
    return;
  m_lastMouse = mouse;
  // The button under the mouse lights up; off them, the last one stays chosen for the keys
  auto local = this->convertToNodeSpace(mouse);
  for (int i = 0; i < static_cast<int>(m_items.size()); i++)
  {
    if (m_items[i]->boundingBox().containsPoint(local) && i != m_shown)
    {
      this->highlight(i, true);
      ms::playSound(Theme::get().hoverSound);
      return;
    }
  }
}
