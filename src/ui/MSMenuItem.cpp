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

  void applyOpacity(CCNode* node, GLubyte opacity)
  {
    if (auto rgba = typeinfo_cast<CCRGBAProtocol*>(node))
      rgba->setOpacity(opacity);
  }

  void applyColor(CCNode* node, ccColor3B color)
  {
    if (auto rgba = typeinfo_cast<CCRGBAProtocol*>(node))
      rgba->setColor(color);
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

  m_label = ms::createLabel(text);
  if (!m_label)
    return false;
  m_label->setScale(theme.itemScale);
  m_label->setAnchorPoint({0, 0.5f});
  auto labelSize = m_label->getScaledContentSize();

  m_accent = ms::createStretched(theme.accentSprite, {theme.accentWidth, labelSize.height}, theme.accentColor,
                                 theme.itemHoverOpacity);
  if (!m_accent)
    return false;
  m_accent->setAnchorPoint({0, 0.5f});
  applyOpacity(m_accent, 0);

  m_content = CCNode::create();
  m_content->setAnchorPoint({0, 0.5f});
  m_content->setContentSize({theme.accentWidth + theme.accentGap + labelSize.width, labelSize.height});
  m_accent->setPosition({0, labelSize.height / 2});
  m_label->setPosition({theme.accentWidth + theme.accentGap, labelSize.height / 2});
  m_content->addChild(m_accent);
  m_content->addChild(m_label);

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
    applyColor(m_label, color);
    applyOpacity(m_label, opacity);
    applyOpacity(m_accent, accent);
    m_content->setPositionX(x);
    m_content->setScale(scale);
    return;
  }
  auto time = theme.hoverDuration;
  run(m_label, CCSpawn::createWithTwoActions(CCTintTo::create(time, color.r, color.g, color.b),
                                             CCFadeTo::create(time, opacity)));
  run(m_accent, CCFadeTo::create(time, accent));
  run(m_content, CCEaseOut::create(CCSpawn::createWithTwoActions(
                                     CCMoveTo::create(time, {x, m_content->getPositionY()}),
                                     CCScaleTo::create(time, scale)),
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
  applyOpacity(m_label, 0);
  run(m_label, CCSequence::createWithTwoActions(CCDelayTime::create(delay),
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
