#include "Theme.hpp"
#include "achievements/MSAchievementList.hpp"
#include "ui/MSBackground.hpp"
#include "ui/MSMenuItem.hpp"
#include "ui/MSMita.hpp"
#include "ui/MSStyle.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

namespace
{
  // The title font's capitals: 17.5 pt high at scale 1, their middle 2.82 pt over the label's
  constexpr float TITLE_CAP = 17.5f;
  constexpr float CAP_RAISE = 2.82f;
  // The item font's capitals: 11.7 pt high at scale 1, their middle 1.88 pt over the label's
  constexpr float FONT_CAP = 11.7f;
  constexpr float FONT_RAISE = 1.88f;
  constexpr int BACKGROUND_Z = -100;
  constexpr int MITA_Z = -50;
  constexpr int MENU_Z = 10;
  constexpr int CURTAIN_Z = 1000;

  bool english() { return Mod::get()->getSettingValue<std::string>("menu-language") == "EN"; }

  // Every achievement GD has now, in its own order (the ones other mods add too), with its status and reward as
  // GD's achievement cell reads them: earned at 100%, the icon "type_id" (no type matched: a cube)
  std::vector<MSAchievement> readAchievements()
  {
    std::vector<MSAchievement> list;
    auto manager = AchievementManager::sharedState();
    if (!manager)
      return list;
    auto sorted = manager->getAllAchievementsSorted(true);
    if (!sorted)
      return list;
    auto text = [](CCDictionary* dict, char const* key) {
      auto value = dict->valueForKey(key);
      return std::string(value ? value->getCString() : "");
    };
    static std::pair<char const*, UnlockType> const TYPES[] = {
      {"ship", UnlockType::Ship},       {"ball", UnlockType::Ball},       {"bird", UnlockType::Bird},
      {"dart", UnlockType::Dart},       {"robot", UnlockType::Robot},     {"spider", UnlockType::Spider},
      {"death", UnlockType::Death},     {"special", UnlockType::Streak},  {"swing", UnlockType::Swing},
      {"jetpack", UnlockType::Jetpack}, {"shipstreak", UnlockType::ShipFire}, {"color", UnlockType::Col1},
      {"color2", UnlockType::Col2},
    };
    for (auto dict : CCArrayExt<CCDictionary*>(sorted))
    {
      if (!dict)
        continue;
      MSAchievement a;
      a.id = text(dict, "identifier");
      a.title = text(dict, "title");
      a.percent = manager->percentForAchievement(a.id.c_str());
      a.earned = a.percent >= 100;
      a.description = text(dict, a.earned ? "achievedDescription" : "unachievedDescription");
      auto icon = text(dict, "icon");
      if (auto split = icon.find('_'); split != std::string::npos)
      {
        auto kind = icon.substr(0, split);
        a.hasReward = true;
        a.item = numFromString<int>(icon.substr(split + 1)).unwrapOr(0);
        a.type = UnlockType::Cube;
        for (auto const& [name, type] : TYPES)
        {
          if (kind == name)
            a.type = type;
        }
      }
      list.push_back(std::move(a));
    }
    return list;
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

// GD's achievements as a screen of MiSide's: the room's background, ACHIEVEMENTS with the count earned, the list on
// the left, BACK under it, Mita on the right. A popup over the menu (so the menu knows it's covered): opaque, the
// menu hidden under it while it's open. Esc and BACK close it
class MSAchievementsLayer : public FLAlertLayer
{
public:
  static MSAchievementsLayer* create()
  {
    auto layer = new MSAchievementsLayer();
    if (layer->setup())
    {
      layer->autorelease();
      return layer;
    }
    delete layer;
    return nullptr;
  }

  ~MSAchievementsLayer() override { CCTouchDispatcher::get()->unregisterForcePrio(this); }

  bool setup()
  {
    auto achievements = readAchievements();
    if (achievements.empty())
      return false;
    CCTouchDispatcher::get()->registerForcePrio(this, 2);
    auto const& theme = Theme::get();
    if (!this->initWithColor(Theme::withAlpha(theme.fadeColor, 255)))
      return false;
    m_noElasticity = true;
    auto winSize = CCDirector::get()->getWinSize();
    auto fromTop = winSize.height - 320;
    auto en = english();

    m_mainLayer = CCLayer::create();
    this->addChild(m_mainLayer);
    m_buttonMenu = CCMenu::create();
    m_buttonMenu->setPosition({0, 0});
    m_mainLayer->addChild(m_buttonMenu, MENU_Z);

    auto strength = static_cast<float>(Mod::get()->getSettingValue<double>("parallax-strength"));
    if (auto background = MSBackground::create(strength))
      m_mainLayer->addChild(background, BACKGROUND_Z);
    if (auto mita = MSMita::create())
      m_mainLayer->addChild(mita, MITA_Z);

    // ACHIEVEMENTS, and how many are earned at the list's right edge on the same line
    ms::Shadow shadow{theme.confirmTitleShadowColor, theme.confirmTitleShadowOpacity, 1,
                      theme.confirmTitleShadowOffset};
    auto capTop = theme.achievementsTitleCapTop + fromTop;
    auto title = ms::createText(en ? "ACHIEVEMENTS" : "ДОСТИЖЕНИЯ", theme.confirmTitleScale, theme.confirmTitleColor,
                                theme.titleFont, shadow);
    if (title.node)
    {
      title.node->setPosition(
        {theme.achievementsTitleLeft, capTop - (TITLE_CAP / 2 + CAP_RAISE) * theme.confirmTitleScale});
      m_mainLayer->addChild(title.node, MENU_Z);
    }
    auto earned = std::count_if(achievements.begin(), achievements.end(), [](auto const& a) { return a.earned; });
    auto counter = ms::createText(fmt::format("{} / {}", earned, achievements.size()), theme.achievementsCounterScale,
                                  theme.achievementsCounterColor, theme.font, ms::Shadow{});
    if (counter.node)
    {
      // Its capitals' bottom on the title's
      auto titleBottom = capTop - TITLE_CAP * theme.confirmTitleScale;
      auto right = theme.achievementsScrollLeft + theme.achievementsScrollWidth;
      counter.node->setPosition({right - counter.node->getContentWidth(),
                                 titleBottom + FONT_CAP * theme.achievementsCounterScale / 2 -
                                   FONT_RAISE * theme.achievementsCounterScale});
      m_mainLayer->addChild(counter.node, MENU_Z);
    }

    m_list = MSAchievementList::create(std::move(achievements), en);
    if (!m_list)
      return false;
    m_mainLayer->addChild(m_list, MENU_Z);

    // BACK under the list, as MiSide's: as wide as the rows and the bar
    auto style = MSMenuItem::Style::confirm();
    if (auto back = MSMenuItem::create(en ? "BACK" : "НАЗАД", nullptr, [this](MSMenuItem*) { this->close(); },
                                       theme.achievementsBackWidth, style))
    {
      back->setPosition({theme.achievementsListLeft - 1, theme.achievementsBackTop + fromTop - style.height / 2});
      m_buttonMenu->addChild(back);
      m_back = back;
    }

    // Comes in from the menu's darkening
    m_curtain = CCLayerColor::create(Theme::withAlpha(theme.fadeColor, theme.fadeOpacity));
    m_curtain->setContentSize(winSize);
    this->addChild(m_curtain, CURTAIN_Z);

    this->setTouchEnabled(true);
    this->setKeypadEnabled(true);
    this->setKeyboardEnabled(true);
    this->scheduleUpdate();
    return true;
  }

  // BACK lights up under the mouse, with its shine, as the menu's rows do
  void update(float) override
  {
    if (!m_back || m_closing)
      return;
    auto mouse = getMousePos();
    if (mouse.equals(m_lastMouse))
      return;
    m_lastMouse = mouse;
    auto local = m_back->getParent()->convertToNodeSpace(mouse);
    auto over = m_back->boundingBox().containsPoint(local);
    if (over == m_back->isHighlighted())
      return;
    m_back->setHighlighted(over, true);
    if (over)
      ms::playSound(Theme::get().hoverSound);
  }

  void show() override
  {
    FLAlertLayer::show();
    auto const& theme = Theme::get();
    // Once it covers the screen, the menu under it hides (its Mita and its effects wait)
    m_curtain->runAction(CCSequence::createWithTwoActions(CCFadeTo::create(theme.fadeBackDuration, 0),
                                                          CallFuncExt::create([this] { this->hideMenu(true); })));
  }

  void registerWithTouchDispatcher() override { CCTouchDispatcher::get()->addTargetedDelegate(this, -500, true); }

  // Every touch is the screen's: nothing reaches the hidden menu
  bool ccTouchBegan(CCTouch*, CCEvent*) override { return true; }
  void ccTouchMoved(CCTouch*, CCEvent*) override {}
  void ccTouchEnded(CCTouch*, CCEvent*) override {}
  void ccTouchCancelled(CCTouch*, CCEvent*) override {}

  void keyBackClicked() override { this->close(); }

  void keyDown(enumKeyCodes key, double) override
  {
    if (key == KEY_Escape || key == CONTROLLER_B)
      return this->close();
    if (m_list)
      m_list->onKey(key);
  }

protected:
  void hideMenu(bool hidden)
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

  // Into the darkening, then gone, the menu showing again as it comes off
  void close()
  {
    if (m_closing)
      return;
    m_closing = true;
    ms::playSound(Theme::get().clickSound);
    if (m_list)
      m_list->setActive(false);
    if (m_back)
      m_back->setHighlighted(false);
    auto const& theme = Theme::get();
    m_curtain->stopAllActions();
    m_curtain->runAction(CCSequence::createWithTwoActions(
      CCFadeTo::create(theme.fadeDuration, theme.fadeOpacity), CallFuncExt::create([this] {
        this->hideMenu(false);
        if (auto scene = this->getParent())
          fadeOff(scene, Theme::get().fadeOpacity);
        this->setKeypadEnabled(false);
        this->setKeyboardEnabled(false);
        this->setTouchEnabled(false);
        this->removeFromParentAndCleanup(true);
      })));
  }

  MSAchievementList* m_list = nullptr;
  MSMenuItem* m_back = nullptr;
  CCPoint m_lastMouse;
  CCLayerColor* m_curtain = nullptr;
  Ref<CCNode> m_hidden;
  bool m_closing = false;
};

// The achievements button (the column's ACHIEVEMENTS presses it): the screen of MiSide's instead of GD's dropdown,
// GD's own when the menu is vanilla or the screen can't be made
class $modify(MSAchievementsMenuLayer, MenuLayer)
{
  void onAchievements(CCObject* sender)
  {
    if (Mod::get()->getSettingValue<bool>("vanilla-menu") || !Theme::get().achievementsEnabled)
      return MenuLayer::onAchievements(sender);
    auto layer = MSAchievementsLayer::create();
    if (!layer)
    {
      log::warn("Achievements: no screen of the mod's, GD's own");
      return MenuLayer::onAchievements(sender);
    }
    layer->show();
  }
};
