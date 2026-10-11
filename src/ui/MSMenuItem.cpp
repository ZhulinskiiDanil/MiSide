#include "MSMenuItem.hpp"

#include "../Theme.hpp"
#include "MSStyle.hpp"

using namespace geode::prelude;

namespace
{
  // One tag for every hover animation, so a new one replaces the running one
  constexpr int HOVER_ACTION = 0x4d53;

  void run(CCNode* node, CCAction* action)
  {
    node->stopActionByTag(HOVER_ACTION);
    action->setTag(HOVER_ACTION);
    node->runAction(action);
  }
} // namespace

MSMenuItem* MSMenuItem::create(std::string const& text, CCMenuItem* target, Callback callback)
{
  auto item = new MSMenuItem();
  if (item->init(text, target, std::move(callback)))
  {
    item->autorelease();
    return item;
  }
  delete item;
  return nullptr;
}

bool MSMenuItem::init(std::string const& text, CCMenuItem* target, Callback callback)
{
  if (!CCMenuItem::initWithTarget(nullptr, nullptr))
    return false;
  auto const& theme = Theme::get();
  m_target = target;
  m_callback = std::move(callback);

  auto label = ms::createText(text, theme.itemScale, theme.itemColor);
  if (!label.node)
    return false;
  m_label = label.label;
  auto labelSize = label.node->getContentSize();

  m_accent = ms::createStretched(theme.accentSprite, {theme.accentWidth, labelSize.height}, theme.accentColor,
                                 theme.itemHoverOpacity);
  if (!m_accent)
    return false;
  m_accent->setAnchorPoint({0, 0.5f});
  ms::cascadeOpacity(m_accent);
  ms::setOpacity(m_accent, 0);

  // Its opacity fades the label, its shadow and the accent bar together
  auto content = CCNodeRGBA::create();
  content->setCascadeOpacityEnabled(true);
  m_content = content;
  m_content->setAnchorPoint({0, 0.5f});
  m_content->setContentSize({theme.accentWidth + theme.accentGap + labelSize.width, labelSize.height});
  m_accent->setPosition({0, labelSize.height / 2});
  label.node->setPosition({theme.accentWidth + theme.accentGap, labelSize.height / 2});
  m_content->addChild(m_accent);
  m_content->addChild(label.node);

  // Wide enough to stay under the mouse when the label moves on hover
  this->setContentSize({m_content->getContentWidth() + theme.hoverShift, labelSize.height});
  this->setAnchorPoint({0, 0.5f});
  m_contentX = 0;
  m_content->setPosition({m_contentX, labelSize.height / 2});
  this->addChild(m_content);

  this->refresh(false);
  return true;
}

CCMenuItem* MSMenuItem::getTarget() const { return m_target; }

bool MSMenuItem::isHighlighted() const { return m_highlighted; }

void MSMenuItem::setHighlighted(bool highlighted)
{
  if (m_highlighted == highlighted)
    return;
  m_highlighted = highlighted;
  this->refresh(true);
}

void MSMenuItem::refresh(bool animate)
{
  auto const& theme = Theme::get();
  auto lit = m_highlighted || m_pressed;
  auto color = m_pressed ? theme.itemPressedColor : m_highlighted ? theme.itemHoverColor : theme.itemColor;
  auto opacity = lit ? theme.itemHoverOpacity : theme.itemOpacity;
  auto x = m_contentX + (lit ? theme.hoverShift : 0);
  auto scale = m_pressed ? theme.pressedScale : 1.f;
  GLubyte accent = lit ? theme.itemHoverOpacity : 0;

  if (!animate || theme.hoverDuration <= 0)
  {
    ms::setColor(m_label, color);
    ms::setOpacity(m_content, opacity);
    ms::setOpacity(m_accent, accent);
    m_content->setPositionX(x);
    m_content->setScale(scale);
    return;
  }
  auto time = theme.hoverDuration;
  run(m_label, CCTintTo::create(time, color.r, color.g, color.b));
  run(m_accent, CCFadeTo::create(time, accent));
  run(m_content, CCEaseOut::create(CCSpawn::create(CCMoveTo::create(time, {x, m_content->getPositionY()}),
                                                   CCScaleTo::create(time, scale), CCFadeTo::create(time, opacity),
                                                   nullptr),
                                   2.f));
}

void MSMenuItem::playIntro(float delay)
{
  auto const& theme = Theme::get();
  if (theme.introDuration <= 0)
    return;
  auto position = this->getPosition();
  this->setPosition(position - CCPoint{theme.introShift, 0});
  this->runAction(CCSequence::createWithTwoActions(
    CCDelayTime::create(delay), CCEaseOut::create(CCMoveTo::create(theme.introDuration, position), 2.f)));

  auto opacity = m_highlighted ? theme.itemHoverOpacity : theme.itemOpacity;
  ms::setOpacity(m_content, 0);
  run(m_content, CCSequence::createWithTwoActions(CCDelayTime::create(delay),
                                                  CCFadeTo::create(theme.introDuration, opacity)));
}

void MSMenuItem::selected()
{
  CCMenuItem::selected();
  m_pressed = true;
  this->refresh(true);
}

void MSMenuItem::unselected()
{
  CCMenuItem::unselected();
  m_pressed = false;
  this->refresh(true);
}

void MSMenuItem::activate()
{
  if (!m_bEnabled || !m_callback)
    return;
  m_callback(this);
}
