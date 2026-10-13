#include "Theme.hpp"
#include "statistics/MSStats.hpp"
#include "ui/MSScreen.hpp"
#include "ui/MSScrollView.hpp"
#include "ui/MSStyle.hpp"
#include "ui/MSValueRow.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

namespace
{
  // The title font's capitals: their middle this far over the label's middle, at scale 1
  constexpr float TITLE_RAISE = 2.82f;

  // GD's own window, once, from our screen's button (the menu's hook lets it through)
  bool s_original = false;
} // namespace

// The statistics as a screen of MiSide's: STATISTICS over GD's numbers in groups (playing, collected, completed,
// ratings), each a row with its name on the left and its number on the right, scrolling when they don't fit; BACK and
// GD's own window (for what other mods add to it) under them; Mita on the right
class MSStatisticsScreen : public MSScreen
{
public:
  static MSStatisticsScreen* create(MenuLayer* menu)
  {
    auto screen = new MSStatisticsScreen();
    if (screen->setup(menu))
    {
      screen->autorelease();
      return screen;
    }
    delete screen;
    return nullptr;
  }

protected:
  bool setup(MenuLayer* menu)
  {
    auto en = english();
    auto groups = ms::stats::read(en);
    if (groups.empty())
      return false;
    if (!this->initScreen(en ? "STATISTICS" : "СТАТИСТИКА"))
      return false;
    m_menu = menu;
    auto const& theme = Theme::get();

    // GD's window: everything it has, the rows other mods add to it too
    this->addBottomButton(en ? "GD STATS" : "ОКНО GD", theme.statsOriginalWidth, [this] {
      this->close([menu = m_menu] {
        if (!menu)
          return;
        s_original = true;
        menu->onStats(nullptr);
        s_original = false;
      });
    });

    auto top = theme.screenTop + fromTop();
    auto bottom = std::min(theme.screenBottom + fromTop(), top - theme.statsRowHeight);
    CCRect view{theme.screenLeft, bottom, theme.screenWidth, top - bottom};
    m_scroll = MSScrollView::create(view, theme.screenScrollLeft, theme.screenScrollWidth,
                                    theme.achievementsScrollTrackColor, theme.achievementsScrollThumbColor);
    if (!m_scroll)
      return false;
    m_scroll->setStep(theme.statsRowHeight + theme.statsRowGap);
    m_mainLayer->addChild(m_scroll, 10);

    auto content = m_scroll->content();
    auto width = theme.screenWidth;
    auto y = 0.f;
    for (size_t g = 0; g < groups.size(); g++)
    {
      if (g > 0)
        y -= theme.statsGroupGap;
      // The group's name, small, in the accent pink
      if (auto name = ms::createText(groups[g].title, theme.statsGroupScale, theme.statsGroupColor, theme.titleFont,
                                     ms::Shadow{}).node)
      {
        name->setPosition({2, y - theme.statsGroupHeight / 2 - TITLE_RAISE * theme.statsGroupScale});
        content->addChild(name);
      }
      y -= theme.statsGroupHeight;
      for (auto const& row : groups[g].rows)
      {
        auto node = ms::createValueRow(row.title, ms::stats::format(row.value, en), width);
        node->setPosition({0, y});
        content->addChild(node);
        y -= theme.statsRowHeight + theme.statsRowGap;
      }
    }
    m_scroll->setContentHeight(-y);
    return true;
  }

  bool onKey(enumKeyCodes key) override
  {
    auto const& theme = Theme::get();
    auto step = theme.statsRowHeight + theme.statsRowGap;
    if (key == KEY_Up || key == CONTROLLER_Up || key == CONTROLLER_LTHUMBSTICK_UP)
      m_scroll->scrollBy(-step);
    else if (key == KEY_Down || key == CONTROLLER_Down || key == CONTROLLER_LTHUMBSTICK_DOWN)
      m_scroll->scrollBy(step);
    else
      return false;
    return true;
  }

  ms::wave::Steps contentSteps() override { return m_scroll->pageSteps(); }

  MSScrollView* m_scroll = nullptr;
  Ref<MenuLayer> m_menu;
};

// The stats button (the column's STATISTICS presses it): the screen of MiSide's instead of GD's dropdown; GD's own
// when the menu is vanilla, the screen is off or can't be made, or our screen's button asks for it
class $modify(MSStatisticsMenuLayer, MenuLayer)
{
  void onStats(CCObject* sender)
  {
    if (s_original || Mod::get()->getSettingValue<bool>("vanilla-menu") || !Theme::get().statsEnabled)
      return MenuLayer::onStats(sender);
    auto screen = MSStatisticsScreen::create(this);
    if (!screen)
    {
      log::warn("Statistics: no screen of the mod's, GD's own");
      return MenuLayer::onStats(sender);
    }
    screen->show();
  }
};
