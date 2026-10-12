#include "Theme.hpp"
#include "statistics/MSStats.hpp"
#include "ui/MSScreen.hpp"
#include "ui/MSScrollView.hpp"
#include "ui/MSStyle.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

namespace
{
  // The fonts' capitals: their middle this far over the label's middle, at scale 1
  constexpr float FONT_RAISE = 1.88f;
  constexpr float TITLE_RAISE = 2.82f;
  // Room inside a row
  constexpr float PADDING = 5;
  constexpr float NAME_GAP = 6;

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
        this->addRow(content, row, y, width, en);
        y -= theme.statsRowHeight + theme.statsRowGap;
      }
    }
    m_scroll->setContentHeight(-y);
    return true;
  }

  // A plate, the name on the left (made smaller if it doesn't fit beside the number), the number on the right
  void addRow(CCNode* content, ms::stats::Row const& row, float top, float width, bool en)
  {
    auto const& theme = Theme::get();
    auto height = theme.statsRowHeight;
    auto middle = top - height / 2;
    auto plate = ms::createPanel(theme.rowSprite, theme.statsRowColor, theme.statsRowOpacity);
    plate->setAnchorPoint({0, 1});
    plate->ignoreAnchorPointForPosition(false);
    plate->setContentSize({width, height});
    plate->setPosition({0, top});
    content->addChild(plate);

    auto value = ms::createText(ms::stats::format(row.value, en), theme.statsValueScale, theme.statsValueColor,
                                theme.titleFont, ms::Shadow{});
    auto valueWidth = 0.f;
    if (value.node)
    {
      valueWidth = value.node->getContentWidth();
      value.node->setPosition({width - PADDING - valueWidth, middle - TITLE_RAISE * theme.statsValueScale});
      content->addChild(value.node, 1);
    }
    auto scale = theme.statsLabelScale;
    auto name = ms::createText(row.title, scale, theme.statsLabelColor);
    auto room = width - PADDING * 2 - valueWidth - NAME_GAP;
    if (name.node && name.node->getContentWidth() > room && room > 0)
    {
      scale *= room / name.node->getContentWidth();
      name = ms::createText(row.title, scale, theme.statsLabelColor);
    }
    if (name.node)
    {
      name.node->setPosition({PADDING, middle - FONT_RAISE * scale});
      content->addChild(name.node, 1);
    }
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
