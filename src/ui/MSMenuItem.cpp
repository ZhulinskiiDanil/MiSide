#include "MSMenuItem.hpp"

#include "../Theme.hpp"
#include "MSStyle.hpp"

#include <cmath>

using namespace geode::prelude;

namespace
{
  // The Exit button's tilt is at its most at this speed, in points a second
  constexpr float TILT_SPEED = 120;

  float mix(float from, float to, float t) { return from + (to - from) * t; }

  ccColor3B mix(ccColor3B from, ccColor3B to, float t)
  {
    auto channel = [t](GLubyte a, GLubyte b) { return static_cast<GLubyte>(std::lround(mix(a, b, t))); };
    return {channel(from.r, to.r), channel(from.g, to.g), channel(from.b, to.b)};
  }

  GLubyte toByte(float value) { return static_cast<GLubyte>(std::lround(std::clamp(value, 0.f, 255.f))); }
} // namespace

MSMenuItem::Style MSMenuItem::Style::row()
{
  auto const& theme = Theme::get();
  Style style;
  style.height = theme.rowHeight;
  style.font = theme.font;
  style.plate = theme.rowColor;
  style.plateOpacity = theme.rowOpacity;
  style.plateLit = theme.rowHoverColor;
  style.plateLitOpacity = theme.rowHoverOpacity;
  style.platePressed = theme.rowPressedColor;
  style.platePressedOpacity = theme.rowPressedOpacity;
  style.text = theme.itemColor;
  style.textLit = theme.itemHoverColor;
  style.textPressed = theme.itemPressedColor;
  style.textScale = theme.itemScale;
  style.textOffset = theme.itemTextOffset;
  style.textPadding = theme.textPadding;
  return style;
}

MSMenuItem::Style MSMenuItem::Style::confirm()
{
  auto const& theme = Theme::get();
  // Lit and pressed as the menu's rows
  auto style = row();
  style.height = theme.confirmRowHeight;
  style.font = theme.confirmTextFont;
  style.plate = theme.confirmRowColor;
  style.plateOpacity = theme.confirmRowOpacity;
  style.text = theme.confirmTextColor;
  style.textScale = theme.confirmTextScale;
  style.textOffset = theme.confirmTextOffset;
  style.textPadding = theme.confirmTextPadding;
  style.textShadow = false;
  return style;
}

MSMenuItem::Style MSMenuItem::Style::pause()
{
  auto const& theme = Theme::get();
  Style style;
  style.height = theme.pauseRowHeight;
  style.font = theme.titleFont;
  style.plate = theme.pauseRowColor;
  style.plateOpacity = theme.pauseRowOpacity;
  style.plateLit = theme.pauseRowHoverColor;
  style.plateLitOpacity = theme.pauseRowHoverOpacity;
  style.platePressed = theme.pauseRowPressedColor;
  style.platePressedOpacity = theme.pauseRowPressedOpacity;
  style.text = theme.pauseTextColor;
  style.textLit = theme.pauseTextHoverColor;
  style.textPressed = theme.pauseTextHoverColor;
  style.textScale = theme.pauseTextScale;
  style.textOffset = theme.pauseTextOffset;
  style.textPadding = theme.pauseTextPadding;
  style.textShadow = false;
  // MiSide's pause rows only get lighter
  style.glow = false;
  style.shine = theme.pauseShineEnabled;
  return style;
}

MSMenuItem* MSMenuItem::create(std::string const& text, CCMenuItem* target, Callback callback, float width,
                               Style const& style)
{
  auto item = new MSMenuItem();
  if (item->init(text, target, std::move(callback), width, style))
  {
    item->autorelease();
    return item;
  }
  delete item;
  return nullptr;
}

bool MSMenuItem::init(std::string const& text, CCMenuItem* target, Callback callback, float width, Style const& style)
{
  if (!CCMenuItem::initWithTarget(nullptr, nullptr))
    return false;
  auto const& theme = Theme::get();
  m_target = target;
  m_callback = std::move(callback);
  m_style = style;
  CCSize size{width, style.height};

  // Holds the plate, the text and the accent bar, and scales them when pressed. Its own opacity stays full: each piece
  // gets its own (see apply())
  auto content = CCNodeRGBA::create();
  content->setContentSize(size);
  content->setAnchorPoint({0.5f, 0.5f});
  content->setPosition(size / 2);
  m_content = content;

  m_plate = ms::createPanel(theme.rowSprite, style.plate, style.plateOpacity);
  if (!m_plate)
    return false;
  m_plate->setAnchorPoint({0, 0});
  m_plate->ignoreAnchorPointForPosition(false);
  m_plate->setContentSize(size);

  // A soft glow around the row when it's lit: under the plate, hidden otherwise
  if (style.glow && theme.rowGlowOpacity > 0 && ms::resourceExists(theme.glowSprite))
  {
    m_glow = ms::createStretched(theme.glowSprite, size + CCSize{theme.rowGlowSpread, theme.rowGlowSpread} * 2,
                                 theme.rowGlowColor, theme.rowGlowOpacity);
    m_glow->setAnchorPoint({0.5f, 0.5f});
    m_glow->setPosition(size / 2);
    ms::setOpacity(m_glow, 0);
    m_content->addChild(m_glow);
  }
  m_content->addChild(m_plate);

  m_textX = style.textPadding;
  if (theme.accentEnabled && theme.accentWidth > 0)
  {
    m_accent = ms::createStretched(theme.accentSprite, {theme.accentWidth, size.height}, theme.accentColor,
                                   theme.itemHoverOpacity);
    if (m_accent)
    {
      m_accent->setAnchorPoint({0, 0.5f});
      m_accent->setPosition({0, size.height / 2});
      ms::setOpacity(m_accent, 0);
      m_content->addChild(m_accent);
      m_textX += theme.accentWidth + theme.accentGap;
    }
  }

  auto shadow = style.textShadow ? ms::Shadow::theme() : ms::Shadow{};
  auto label = ms::createText(text, style.textScale, style.text, style.font, shadow);
  if (!label.node)
    return false;
  m_text = label.node;
  m_label = label.label;
  m_text->setPosition({m_textX, size.height / 2 + style.textOffset});
  m_content->addChild(m_text);
  this->addShine(text);

  // All of the row is clickable
  this->setContentSize(size);
  this->setAnchorPoint({0, 0.5f});
  this->addChild(m_content);

  m_unavailable = this->isUnavailable();
  this->refresh(false);
  // A vanilla button can be turned off and on later: the row follows it
  this->scheduleUpdate();
  return true;
}

CCMenuItem* MSMenuItem::getTarget() const { return m_target; }

bool MSMenuItem::isHighlighted() const { return m_highlighted; }

// With no vanilla button it's always there: it only calls back
bool MSMenuItem::isUnavailable() const { return m_target && !m_target->isEnabled(); }

void MSMenuItem::setHighlighted(bool highlighted, bool shine)
{
  if (m_highlighted == highlighted)
    return;
  m_highlighted = highlighted;
  this->refresh(true);
  // A new pass on lighting up (from its start, whatever the last one did); leaving fades the pass out
  if (highlighted && shine && m_shine)
    m_shinePass.start();
  else if (!highlighted)
    m_shinePass.leave();
}

// ! --- The shine --- !

void MSMenuItem::addShine(std::string const& text)
{
  auto const& theme = Theme::get();
  if (!m_style.shine || !theme.shineEnabled || theme.shineDuration <= 0 || theme.shineIntensity <= 0)
    return;
  // Made once from a white render of the label: the core over the label, the glow under it and its shadow. The
  // hit area stays the row
  auto shine = MSShine::create(text, m_style.textScale, m_style.font);
  if (!shine.core || !shine.glow)
    return;
  m_shine = shine.core;
  m_shineGlow = shine.glow;
  m_text->addChild(m_shineGlow, -1);
  m_text->addChild(m_shine, 1);
}

// ! --- Exit leaning away --- !

void MSMenuItem::setDodgeHome(CCPoint home)
{
  // Turned and stretched around its middle
  m_dodging = true;
  m_dodgeHome = home;
  this->setAnchorPoint({0.5f, 0.5f});
  this->setPosition(home);
}

void MSMenuItem::stepDodge(float dt, CCPoint const* mouse)
{
  if (!m_dodging)
    return;
  auto const& theme = Theme::get();
  DodgeSpring::Params params;
  params.radius = theme.exitDodgeRadius;
  params.maxOffset = theme.exitDodgeMax;
  params.vertical = theme.exitDodgeVertical;
  params.halfWidth = this->getContentWidth() / 2;
  params.halfHeight = this->getContentHeight() / 2;
  params.stiffness = theme.exitDodgeStiffness;
  params.damping = theme.exitDodgeDamping;

  // Pressed: it holds still until the press ends, so the click lands
  if (this->isSelected())
    m_dodge.hold();
  else
  {
    DodgeSpring::Vec fromHome;
    auto use = mouse && theme.exitDodgeEnabled;
    if (use)
      fromHome = {mouse->x - m_dodgeHome.x, mouse->y - m_dodgeHome.y};
    m_dodge.advance(dt, m_dodge.target(use ? &fromHome : nullptr, params), params);
  }

  // Always on the screen
  auto offset = m_dodge.offset();
  auto winSize = CCDirector::get()->getWinSize();
  CCPoint position{std::clamp(m_dodgeHome.x + offset.x, params.halfWidth, winSize.width - params.halfWidth),
                   std::clamp(m_dodgeHome.y + offset.y, params.halfHeight, winSize.height - params.halfHeight)};
  this->setPosition(position);

  // Stretched along the push, a little narrower across it, tilted by its speed: the pull of the spring
  auto length = std::hypot(offset.x, offset.y);
  auto stretch = params.maxOffset > 0 ? theme.exitDodgeStretch * std::min(length / params.maxOffset, 1.f) : 0.f;
  auto along = length > 0 ? std::abs(offset.x) / length : 0.f;
  auto across = length > 0 ? std::abs(offset.y) / length : 0.f;
  this->setScaleX(1 + stretch * along - stretch * across / 2);
  this->setScaleY(1 + stretch * across - stretch * along / 2);
  this->setRotation(theme.exitDodgeTilt * std::clamp(m_dodge.velocity().x / TILT_SPEED, -1.f, 1.f));
}

void MSMenuItem::update(float dt)
{
  // The shine's pass, by the time that passed
  if (m_shine)
  {
    auto const& theme = Theme::get();
    m_shinePass.advance(dt, theme.shineDuration, theme.shineFadeOut);
    auto strength = m_shinePass.active() ? theme.shineIntensity * m_shinePass.fade() : 0.f;
    // From fully hidden to fully gone, the glow's reach past the letters counted too
    auto travel = theme.shineBandWidth + 2 * m_shine->margin();
    auto center = m_shinePass.center(theme.shineDuration, travel);
    // Held at one point of the pass while the item is lit, to compare (shine-freeze, -1: off)
    if (theme.shineFreeze >= 0)
    {
      center = -travel / 2 + std::clamp(theme.shineFreeze, 0.f, 1.f) * (1 + travel);
      strength = m_highlighted ? theme.shineIntensity : 0.f;
    }
    // Faded with the item
    strength *= m_visibility;
    m_shine->setPass(center, strength);
    m_shineGlow->setPass(center, strength);
  }

  // The look's change and the visibility's, by the time that passed
  auto changed = false;
  if (m_lookTime < m_lookDuration)
  {
    m_lookTime = std::min(m_lookTime + dt, m_lookDuration);
    auto t = m_lookTime / m_lookDuration;
    m_look = {mix(m_from.plate, m_to.plate, t),     mix(m_from.plateOpacity, m_to.plateOpacity, t),
              mix(m_from.text, m_to.text, t),       mix(m_from.textOpacity, m_to.textOpacity, t),
              mix(m_from.accent, m_to.accent, t),   mix(m_from.glow, m_to.glow, t),
              mix(m_from.textX, m_to.textX, t),     mix(m_from.scale, m_to.scale, t)};
    changed = true;
  }
  if (m_visibility != m_visibilityTo)
  {
    if (m_visibilityDelay > 0)
      m_visibilityDelay -= dt;
    else
    {
      m_visibilityTime = std::min(m_visibilityTime + dt, m_visibilityDuration);
      auto t = m_visibilityDuration > 0 ? m_visibilityTime / m_visibilityDuration : 1.f;
      m_visibility = t >= 1 ? m_visibilityTo : mix(m_visibilityFrom, m_visibilityTo, t);
      changed = true;
    }
  }
  if (changed)
    this->apply();

  auto unavailable = this->isUnavailable();
  if (unavailable == m_unavailable)
    return;
  m_unavailable = unavailable;
  this->refresh(true);
}

MSMenuItem::Look MSMenuItem::target() const
{
  auto const& theme = Theme::get();
  ccColor3B row = m_style.plate;
  GLubyte rowOpacity = m_style.plateOpacity;
  ccColor3B text = m_style.text;
  GLubyte textOpacity = theme.itemOpacity;
  auto lit = false;
  if (m_unavailable)
  {
    row = theme.rowDisabledColor;
    rowOpacity = theme.rowDisabledOpacity;
  }
  else if (m_pressed)
  {
    row = m_style.platePressed;
    rowOpacity = m_style.platePressedOpacity;
    text = m_style.textPressed;
    textOpacity = theme.itemHoverOpacity;
    lit = true;
  }
  else if (m_highlighted)
  {
    row = m_style.plateLit;
    rowOpacity = m_style.plateLitOpacity;
    text = m_style.textLit;
    textOpacity = theme.itemHoverOpacity;
    lit = true;
  }
  Look look;
  look.plate = row;
  look.plateOpacity = rowOpacity;
  look.text = text;
  look.textOpacity = textOpacity;
  look.accent = lit ? theme.itemHoverOpacity : 0;
  look.glow = lit ? theme.rowGlowOpacity : 0;
  look.textX = m_textX + (lit ? theme.hoverShift : 0);
  look.scale = m_pressed ? theme.pressedScale : 1.f;
  return look;
}

void MSMenuItem::refresh(bool animate)
{
  auto const& theme = Theme::get();
  m_from = m_look;
  m_to = this->target();
  m_lookTime = 0;
  m_lookDuration = animate ? std::max(theme.hoverDuration, 0.f) : 0.f;
  if (m_lookDuration <= 0)
  {
    m_look = m_to;
    this->apply();
  }
}

void MSMenuItem::apply()
{
  auto const& look = m_look;
  auto visible = std::clamp(m_visibility, 0.f, 1.f);
  ms::setColor(m_plate, look.plate);
  ms::setOpacity(m_plate, toByte(look.plateOpacity * visible));
  ms::setColor(m_label, look.text);
  // The text node passes it on to the label and its shadow, each keeping its own
  ms::setOpacity(m_text, toByte(look.textOpacity * visible));
  ms::setOpacity(m_accent, toByte(look.accent * visible));
  ms::setOpacity(m_glow, toByte(look.glow * visible));
  m_text->setPositionX(look.textX);
  m_content->setScale(look.scale);
}

void MSMenuItem::resetLook()
{
  m_highlighted = false;
  m_pressed = false;
  m_shinePass.stop();
  if (m_shine)
  {
    m_shine->setPass(0, 0);
    m_shineGlow->setPass(0, 0);
  }
  this->refresh(false);
}

void MSMenuItem::playIntro(float delay)
{
  auto const& theme = Theme::get();
  if (theme.introDuration <= 0)
    return;
  if (theme.introShift != 0)
  {
    auto position = this->getPosition();
    this->setPosition(position - CCPoint{theme.introShift, 0});
    this->runAction(CCSequence::createWithTwoActions(
      CCDelayTime::create(delay), CCEaseOut::create(CCMoveTo::create(theme.introDuration, position), 2.f)));
  }
  m_visibility = 0;
  m_visibilityFrom = 0;
  m_visibilityTo = 1;
  m_visibilityTime = 0;
  m_visibilityDuration = theme.introDuration;
  m_visibilityDelay = std::max(delay, 0.f);
  this->apply();
}

void MSMenuItem::fadeTo(float time, GLubyte opacity)
{
  m_visibilityFrom = m_visibility;
  m_visibilityTo = opacity / 255.f;
  m_visibilityTime = 0;
  m_visibilityDuration = std::max(time, 0.f);
  m_visibilityDelay = 0;
  if (m_visibilityDuration <= 0)
  {
    m_visibility = m_visibilityTo;
    this->apply();
  }
}

void MSMenuItem::selected()
{
  CCMenuItem::selected();
  m_pressed = !m_unavailable;
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
  if (!m_bEnabled || m_unavailable || !m_callback)
    return;
  m_callback(this);
}
