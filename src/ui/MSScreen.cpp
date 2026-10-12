#include "MSScreen.hpp"

#include "../Theme.hpp"
#include "MSBackground.hpp"
#include "MSMita.hpp"
#include "MSStyle.hpp"

#include <limits>

using namespace geode::prelude;

namespace
{
  // The title font's capitals: 17.5 pt high at scale 1, their middle 2.82 pt over the label's
  constexpr float TITLE_CAP = 17.5f;
  constexpr float CAP_RAISE = 2.82f;
  constexpr int BACKGROUND_Z = -100;
  constexpr int MITA_Z = -50;
  constexpr int MENU_Z = 10;
  constexpr int CURTAIN_Z = 1000;
  // Between the bottom row's buttons
  constexpr float BUTTON_GAP = 4;
  // An overlay's shade under its column: its darkest and how far right it reaches
  constexpr GLubyte OVERLAY_SHADE = 225;
  constexpr float OVERLAY_SHADE_WIDTH = 330;

  // The menu's darkening, coming off over what's under it after the screen has gone
  void fadeOff(CCNode* scene, GLubyte from)
  {
    auto const& theme = Theme::get();
    auto fade = CCLayerColor::create(Theme::withAlpha(theme.fadeColor, from));
    scene->addChild(fade, std::numeric_limits<int>::max());
    fade->runAction(CCSequence::createWithTwoActions(CCFadeTo::create(theme.fadeBackDuration, 0),
                                                     CCRemoveSelf::create()));
  }
} // namespace

MSScreen::~MSScreen() { CCTouchDispatcher::get()->unregisterForcePrio(this); }

bool MSScreen::english() { return Mod::get()->getSettingValue<std::string>("menu-language") == "EN"; }

float MSScreen::fromTop() { return CCDirector::get()->getWinSize().height - 320; }

bool MSScreen::isClosing() const { return m_closing; }

bool MSScreen::initScreen(std::string const& title, bool overlay)
{
  // The menus made from here on take touches before the screen itself
  CCTouchDispatcher::get()->registerForcePrio(this, 2);
  auto const& theme = Theme::get();
  m_overlay = overlay;
  if (!this->initWithColor(Theme::withAlpha(theme.fadeColor, overlay ? 0 : 255)))
    return false;
  m_noElasticity = true;
  auto winSize = CCDirector::get()->getWinSize();

  m_mainLayer = CCLayer::create();
  this->addChild(m_mainLayer);
  m_buttonMenu = CCMenu::create();
  m_buttonMenu->setPosition({0, 0});
  m_mainLayer->addChild(m_buttonMenu, MENU_Z);

  auto strength = static_cast<float>(Mod::get()->getSettingValue<double>("parallax-strength"));
  if (overlay)
  {
    // The window shows through, darker under the column
    auto shade = CCLayerGradient::create(Theme::withAlpha(theme.fadeColor, OVERLAY_SHADE),
                                         Theme::withAlpha(theme.fadeColor, 0), {1, 0});
    shade->setContentSize({OVERLAY_SHADE_WIDTH, winSize.height});
    m_mainLayer->addChild(shade, BACKGROUND_Z);
    m_shade = shade;
  }
  else if (auto background = MSBackground::create(strength))
    m_mainLayer->addChild(background, BACKGROUND_Z);
  if (!overlay)
  {
    if (auto mita = MSMita::create())
      m_mainLayer->addChild(mita, MITA_Z);
  }

  ms::Shadow shadow{theme.confirmTitleShadowColor, theme.confirmTitleShadowOpacity, 1,
                    theme.confirmTitleShadowOffset};
  auto text = ms::createText(title, theme.confirmTitleScale, theme.confirmTitleColor, theme.titleFont, shadow);
  if (text.node)
  {
    auto capTop = theme.screenTitleCapTop + fromTop();
    text.node->setPosition({theme.screenTitleLeft, capTop - (TITLE_CAP / 2 + CAP_RAISE) * theme.confirmTitleScale});
    m_mainLayer->addChild(text.node, MENU_Z);
  }

  m_bottomRight = theme.screenLeft - 1 - BUTTON_GAP;
  this->addBottomButton(english() ? "BACK" : "НАЗАД", theme.screenBackWidth, [this] { this->close(); });

  // Comes in from the menu's darkening (an overlay: at once)
  m_curtain = CCLayerColor::create(Theme::withAlpha(theme.fadeColor, overlay ? 0 : theme.fadeOpacity));
  m_curtain->setContentSize(winSize);
  this->addChild(m_curtain, CURTAIN_Z);

  this->setTouchEnabled(true);
  this->setKeypadEnabled(true);
  this->setKeyboardEnabled(true);
  this->scheduleUpdate();
  return true;
}

MSMenuItem* MSScreen::addBottomButton(std::string const& text, float width, std::function<void()> action)
{
  auto const& theme = Theme::get();
  auto style = MSMenuItem::Style::confirm();
  auto item = MSMenuItem::create(text, nullptr,
                                 [this, action = std::move(action)](MSMenuItem*) {
                                   if (m_closing)
                                     return;
                                   ms::playSound(Theme::get().clickSound);
                                   if (action)
                                     action();
                                 },
                                 width, style);
  if (!item)
    return nullptr;
  item->setPosition({m_bottomRight + BUTTON_GAP, theme.screenBackTop + fromTop() - style.height / 2});
  m_bottomRight += BUTTON_GAP + width;
  m_buttonMenu->addChild(item);
  m_bottom.push_back(item);
  return item;
}

void MSScreen::show()
{
  FLAlertLayer::show();
  if (m_overlay)
    return;
  auto const& theme = Theme::get();
  // Once it covers the screen, the menu under it hides (its Mita and its effects wait)
  m_curtain->runAction(CCSequence::createWithTwoActions(CCFadeTo::create(theme.fadeBackDuration, 0),
                                                        CallFuncExt::create([this] { this->hideMenu(true); })));
}

void MSScreen::registerWithTouchDispatcher() { CCTouchDispatcher::get()->addTargetedDelegate(this, -500, true); }

bool MSScreen::ccTouchBegan(CCTouch*, CCEvent*) { return !m_overlay; }

void MSScreen::ccTouchMoved(CCTouch*, CCEvent*) {}

void MSScreen::ccTouchEnded(CCTouch*, CCEvent*) {}

void MSScreen::ccTouchCancelled(CCTouch*, CCEvent*) {}

void MSScreen::keyBackClicked()
{
  auto frame = CCDirector::get()->getTotalFrames();
  if (frame == m_backFrame || m_closing)
    return;
  m_backFrame = frame;
  if (this->onBack())
    return;
  ms::playSound(Theme::get().clickSound);
  this->close();
}

bool MSScreen::onBack() { return false; }

void MSScreen::setBottomEnabled(bool enabled)
{
  for (auto item : m_bottom)
  {
    item->setEnabled(enabled);
    if (!enabled)
      item->setHighlighted(false);
  }
}

void MSScreen::keyDown(enumKeyCodes key, double)
{
  if (key == KEY_Escape || key == CONTROLLER_B)
    return this->keyBackClicked();
  if (!m_closing)
    this->onKey(key);
}

bool MSScreen::onKey(enumKeyCodes) { return false; }

// The bottom row's buttons light up under the mouse, with their shine, as the menu's rows do
void MSScreen::update(float)
{
  if (m_closing || m_bottom.empty())
    return;
  auto mouse = getMousePos();
  if (mouse.equals(m_lastMouse))
    return;
  m_lastMouse = mouse;
  auto local = m_buttonMenu->convertToNodeSpace(mouse);
  for (auto item : m_bottom)
  {
    auto over = item->isVisible() && item->isEnabled() && item->boundingBox().containsPoint(local);
    if (over == item->isHighlighted())
      continue;
    item->setHighlighted(over, true);
    if (over)
      ms::playSound(Theme::get().hoverSound);
  }
}

void MSScreen::hideMenu(bool hidden)
{
  auto scene = this->getParent();
  if (!scene)
    return;
  if (hidden)
  {
    for (auto child : CCArrayExt<CCNode*>(scene->getChildren()))
    {
      if (auto menu = typeinfo_cast<MenuLayer*>(child); menu && menu->isVisible())
      {
        menu->setVisible(false);
        m_hidden = menu;
      }
    }
  }
  else if (m_hidden)
  {
    m_hidden->setVisible(true);
    m_hidden = nullptr;
  }
}

void MSScreen::close(std::function<void()> after)
{
  if (m_closing)
    return;
  m_closing = true;
  for (auto item : m_bottom)
    item->setHighlighted(false);
  auto const& theme = Theme::get();
  m_curtain->stopAllActions();
  m_curtain->runAction(CCSequence::createWithTwoActions(
    CCFadeTo::create(m_overlay ? 0 : theme.fadeDuration, m_overlay ? 0 : theme.fadeOpacity),
    CallFuncExt::create([this, after = std::move(after)] {
      // Kept alive to the end of this, whatever removing it releases
      Ref<MSScreen> self = this;
      auto then = after;
      this->hideMenu(false);
      if (auto scene = this->getParent(); scene && !m_overlay)
        fadeOff(scene, Theme::get().fadeOpacity);
      this->setKeypadEnabled(false);
      this->setKeyboardEnabled(false);
      this->setTouchEnabled(false);
      this->removeFromParentAndCleanup(true);
      if (then)
        then();
      else
        this->closed();
    })));
}

void MSScreen::closed() {}
