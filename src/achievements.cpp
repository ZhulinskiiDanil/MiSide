#include "Theme.hpp"
#include "achievements/MSAchievementList.hpp"
#include "ui/MSScreen.hpp"
#include "ui/MSStyle.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

namespace
{
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

} // namespace

// GD's achievements as a page of the menu: ACHIEVEMENTS with the count earned at the list's right edge on its line,
// the list under it, BACK under the list. Esc and BACK close it
class MSAchievementsScreen : public MSScreen
{
public:
  static MSAchievementsScreen* create()
  {
    auto screen = new MSAchievementsScreen();
    if (screen->setup())
    {
      screen->autorelease();
      return screen;
    }
    delete screen;
    return nullptr;
  }

protected:
  bool setup()
  {
    auto achievements = readAchievements();
    if (achievements.empty())
      return false;
    auto en = english();
    if (!this->initScreen(en ? "ACHIEVEMENTS" : "ДОСТИЖЕНИЯ"))
      return false;
    auto const& theme = Theme::get();

    // How many are earned, its middle on the title's
    auto earned = std::count_if(achievements.begin(), achievements.end(), [](auto const& a) { return a.earned; });
    auto counter = ms::createText(fmt::format("{} / {}", earned, achievements.size()), theme.achievementsCounterScale,
                                  theme.achievementsCounterColor, theme.font, ms::Shadow{});
    if (counter.node && !m_titleNodes.empty())
    {
      auto title = m_titleNodes.front();
      auto middle = title->getPositionY() - title->getScaledContentSize().height / 2;
      auto right = theme.achievementsScrollLeft + theme.achievementsScrollWidth;
      counter.node->setPosition({right - counter.node->getContentWidth(), middle});
      m_mainLayer->addChild(counter.node, 10);
      this->addToTitle(counter.node);
    }

    m_list = MSAchievementList::create(std::move(achievements), en);
    if (!m_list)
      return false;
    m_mainLayer->addChild(m_list, 10);
    return true;
  }

  ms::wave::Steps contentSteps() override { return m_list->pageSteps(); }

  bool onKey(enumKeyCodes key) override { return m_list->onKey(key); }

  void closing() override { m_list->setActive(false); }

  MSAchievementList* m_list = nullptr;
};

// The achievements button (the column's ACHIEVEMENTS presses it): the screen of MiSide's instead of GD's dropdown,
// GD's own when the menu is vanilla or the screen can't be made
class $modify(MSAchievementsMenuLayer, MenuLayer)
{
  void onAchievements(CCObject* sender)
  {
    if (Mod::get()->getSettingValue<bool>("vanilla-menu") || !Theme::get().achievementsEnabled)
      return MenuLayer::onAchievements(sender);
    auto screen = MSAchievementsScreen::create();
    if (!screen)
    {
      log::warn("Achievements: no screen of the mod's, GD's own");
      return MenuLayer::onAchievements(sender);
    }
    screen->show();
  }
};
