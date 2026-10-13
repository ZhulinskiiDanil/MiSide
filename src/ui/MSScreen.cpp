#include "MSScreen.hpp"

#include "../Theme.hpp"
#include "MSBackground.hpp"
#include "MSLogo.hpp"
#include "MSMenuColumn.hpp"
#include "MSMita.hpp"
#include "MSStyle.hpp"

#include <algorithm>

#include <limits>

using namespace geode::prelude;

namespace
{
  constexpr int BACKGROUND_Z = -100;
  constexpr int MITA_Z = -50;
  constexpr int MENU_Z = 10;
  constexpr int CURTAIN_Z = 1000;
  // Between the bottom row's buttons
  constexpr float BUTTON_GAP = 4;
  // An overlay's shade under its column: its darkest and how far right it reaches
  constexpr GLubyte OVERLAY_SHADE = 225;
  constexpr float OVERLAY_SHADE_WIDTH = 330;
  // The main menu's nodes that stay as a page comes in over it; the others (the column's rows, the tray, the
  // profile card, the mods' buttons) go with the page
  constexpr char const* MENU_STAYS[] = {"column", "background", "mita", "transition"};

  // How high its top is on the screen: the order the page's steps go in
  float topOf(ms::wave::Step const& step)
  {
    auto top = -1e9f;
    for (auto node : step.nodes)
    {
      if (auto parent = node->getParent())
      {
        auto box = node->boundingBox();
        top = std::max(top, parent->convertToWorldSpace({box.getMinX(), box.getMaxY()}).y);
      }
    }
    return top;
  }

  // The main menu's page: MAIN MENU and its rows, then everything else of it that goes (only fading)
  ms::wave::Steps menuSteps(MenuLayer* menu)
  {
    auto column = typeinfo_cast<MSMenuColumn*>(menu->getChildByID(std::string(GEODE_MOD_ID) + "/column"));
    if (!column)
      return {};
    auto steps = column->pageSteps();
    ms::wave::Step rest;
    rest.slide = false;
    for (auto child : CCArrayExt<CCNode*>(menu->getChildren()))
    {
      auto stays = std::any_of(std::begin(MENU_STAYS), std::end(MENU_STAYS), [&](char const* id) {
        return child->getID() == std::string(GEODE_MOD_ID) + "/" + id;
      });
      if (!stays && ms::wave::shows(child))
        rest.nodes.push_back(child);
    }
    if (!rest.nodes.empty())
      steps.push_back(std::move(rest));
    return steps;
  }

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
  if (!overlay)
  {
    m_host = this->findHost();
    m_inPlace = m_host.lock() != nullptr;
  }
  // In place, what's under it shows through: the page it goes over, the logo, the background, Mita
  if (!this->initWithColor(Theme::withAlpha(theme.fadeColor, overlay || m_inPlace ? 0 : 255)))
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
  else if (!m_inPlace)
  {
    // Its own, as the main menu has them
    if (auto background = MSBackground::create(strength))
      m_mainLayer->addChild(background, BACKGROUND_Z);
    if (auto mita = MSMita::create())
      m_mainLayer->addChild(mita, MITA_Z);
    if (Mod::get()->getSettingValue<bool>("show-title"))
    {
      if (auto logo = MSLogo::create())
      {
        logo->fit(1);
        logo->ignoreAnchorPointForPosition(false);
        logo->setAnchorPoint({0, 0});
        logo->setPosition({theme.logoLeft, theme.logoBottom + fromTop()});
        m_mainLayer->addChild(logo, MENU_Z);
      }
    }
  }

  // Where MAIN MENU is, as it looks
  auto text = ms::createText(title, theme.subtitleScale, theme.subtitleColor, theme.titleFont);
  if (text.node)
  {
    text.node->setAnchorPoint({0, 1});
    text.node->setPosition({theme.columnLeft, theme.subtitleTop + fromTop()});
    m_mainLayer->addChild(text.node, MENU_Z);
    m_titleNodes.push_back(text.node);
  }

  m_bottomRight = theme.screenLeft - 1 - BUTTON_GAP;
  this->addBottomButton(english() ? "BACK" : "НАЗАД", theme.screenBackWidth, [this] { this->close(); });

  // Comes in from the menu's darkening (an overlay or in place: at once)
  m_curtain = CCLayerColor::create(Theme::withAlpha(theme.fadeColor, overlay || m_inPlace ? 0 : theme.fadeOpacity));
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

void MSScreen::addToPage(CCNode* node)
{
  if (node)
    m_pageNodes.push_back(node);
}

void MSScreen::addToTitle(CCNode* node)
{
  if (node)
    m_titleNodes.push_back(node);
}

ms::wave::Steps MSScreen::contentSteps() { return {}; }

ms::wave::Steps MSScreen::pageSteps()
{
  ms::wave::Steps steps;
  ms::wave::Step title;
  for (auto node : m_titleNodes)
  {
    if (ms::wave::shows(node))
      title.nodes.push_back(node);
  }
  if (!title.nodes.empty())
    steps.push_back(std::move(title));
  // The content from the top down, wherever it came from
  auto content = this->contentSteps();
  for (auto node : m_pageNodes)
  {
    if (ms::wave::shows(node))
      content.push_back({{node}});
  }
  std::erase_if(content, [](auto const& step) { return step.nodes.empty(); });
  std::stable_sort(content.begin(), content.end(), [](auto const& a, auto const& b) { return topOf(a) > topOf(b); });
  steps.insert(steps.end(), content.begin(), content.end());
  ms::wave::Step bottom;
  for (auto item : m_bottom)
  {
    if (ms::wave::shows(item))
      bottom.nodes.push_back(item);
  }
  if (!bottom.nodes.empty())
    steps.push_back(std::move(bottom));
  return steps;
}

CCNode* MSScreen::findHost()
{
  auto scene = CCDirector::get()->getRunningScene();
  if (!scene)
    return nullptr;
  // From the top: the first page there, unless a popup of GD's is over it
  std::vector<CCNode*> children;
  for (auto child : CCArrayExt<CCNode*>(scene->getChildren()))
    children.push_back(child);
  for (auto it = children.rbegin(); it != children.rend(); ++it)
  {
    auto child = *it;
    if (child == this || !child->isVisible())
      continue;
    if (auto screen = typeinfo_cast<MSScreen*>(child))
    {
      if (screen->m_closing || screen->m_overlay)
        continue;
      return screen;
    }
    if (typeinfo_cast<FLAlertLayer*>(child))
      return nullptr;
    if (auto menu = typeinfo_cast<MenuLayer*>(child))
    {
      auto column = menu->getChildByID(std::string(GEODE_MOD_ID) + "/column");
      return column && column->isVisible() ? menu : nullptr;
    }
  }
  return nullptr;
}

ms::wave::Steps MSScreen::hostSteps()
{
  auto host = m_host.lock();
  if (!host)
    return {};
  if (auto screen = typeinfo_cast<MSScreen*>(host.data()))
    return screen->pageSteps();
  if (auto menu = typeinfo_cast<MenuLayer*>(host.data()))
    return menuSteps(menu);
  return {};
}

void MSScreen::show()
{
  FLAlertLayer::show();
  if (m_overlay)
    return;
  auto const& theme = Theme::get();
  if (m_inPlace)
  {
    // The page under it goes right as this one comes in from the left into its places
    ms::wave::out(this->hostSteps());
    ms::wave::in(this->pageSteps(), theme.pageInDelay);
    return;
  }
  // Once it covers the screen, the menu under it hides (its Mita and its effects wait)
  m_curtain->runAction(CCSequence::createWithTwoActions(CCFadeTo::create(theme.fadeBackDuration, 0),
                                                        CallFuncExt::create([this] { this->hideMenu(true); })));
  ms::wave::in(this->pageSteps());
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
  this->closing();
  auto const& theme = Theme::get();
  if (m_inPlace)
  {
    // Out to the right as the page under it comes back from the left, as it went
    auto gone = ms::wave::out(this->pageSteps());
    ms::wave::in(this->hostSteps(), theme.pageInDelay);
    this->runAction(CCSequence::createWithTwoActions(
      CCDelayTime::create(gone), CallFuncExt::create([this, after = std::move(after)] { this->remove(after); })));
    return;
  }
  m_curtain->stopAllActions();
  m_curtain->runAction(CCSequence::createWithTwoActions(
    CCFadeTo::create(m_overlay ? 0 : theme.fadeDuration, m_overlay ? 0 : theme.fadeOpacity),
    CallFuncExt::create([this, after = std::move(after)] {
      this->hideMenu(false);
      if (auto scene = this->getParent(); scene && !m_overlay)
        fadeOff(scene, Theme::get().fadeOpacity);
      this->remove(after);
    })));
}

void MSScreen::remove(std::function<void()> after)
{
  // Kept alive to the end of this, whatever removing it releases
  Ref<MSScreen> self = this;
  this->setKeypadEnabled(false);
  this->setKeyboardEnabled(false);
  this->setTouchEnabled(false);
  this->removeFromParentAndCleanup(true);
  if (after)
    after();
  else
    this->closed();
}

void MSScreen::closing() {}

void MSScreen::closed() {}
