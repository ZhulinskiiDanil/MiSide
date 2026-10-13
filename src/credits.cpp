#include "credits/MSCredits.hpp"

#include "Theme.hpp"
#include "ui/MSScreen.hpp"
#include "ui/MSScrollView.hpp"
#include "ui/MSStyle.hpp"

#include <Geode/Geode.hpp>

#include <vector>

using namespace geode::prelude;

namespace
{
  // The title font's capitals' middle over the label's middle, at scale 1
  constexpr float TITLE_RAISE = 2.82f;
  constexpr float LINE_GAP = 2;

  struct Entry
  {
    // A group's name, then its lines
    char const* ru;
    char const* en;
    std::vector<char const*> linesRu;
    std::vector<char const*> linesEn;
  };

  // From LICENSES.md and resources/mita-closeup/ATTRIBUTION.txt, as they are: who, what, and the license (or that
  // the rights are MiSide's)
  std::vector<Entry> const& entries()
  {
    static std::vector<Entry> const ENTRIES = {
      {"МИТА",
       "MITA",
       {"3D-модель «Mita»: Elite Big Speakerman (sketchfab.com/EvesStudio2008)", "Лицензия CC BY 4.0",
        "3D-анимация в реальном времени; атласы — резервный вариант"},
       {"3D model \"Mita\": Elite Big Speakerman (sketchfab.com/EvesStudio2008)", "License: CC BY 4.0",
        "Real-time 3D animation; rendered atlases remain as a fallback"}},
      {"ШРИФТ",
       "FONT",
       {"M PLUS Rounded 1c Bold и ExtraBold: Coji Morishita, M+ Fonts Project", "Лицензия SIL Open Font License 1.1"},
       {"M PLUS Rounded 1c Bold and ExtraBold: Coji Morishita, M+ Fonts Project", "License: SIL Open Font License 1.1"}},
      {"MISIDE",
       "MISIDE",
       {"Звуки, музыка, шутки экрана загрузки и интро — из MiSide (AIHASTO)",
        "Права у правообладателей MiSide; логотип AIHASTO — их товарный знак"},
       {"Sounds, music, the loading screen's jokes and the intro are MiSide's (AIHASTO)",
        "The rights are MiSide's owners'; the AIHASTO logo is their trademark"}},
      {"ЗВУКИ KENNEY",
       "KENNEY SOUNDS",
       {"UI Audio (rollover2, click1): Kenney, CC0 1.0"},
       {"UI Audio (rollover2, click1): Kenney, CC0 1.0"}},
      {"МОД",
       "THE MOD",
       {"Логотип в стиле меню — от автора мода", "Узор, свечение и плашки нарисованы tools/make_placeholders.py"},
       {"The menu-style logo is the mod author's", "The pattern, glows and plates are drawn by tools/make_placeholders.py"}},
      {"УПРАВЛЕНИЕ",
       "CONTROLS",
       {"Esc или B — назад; стрелки — выбор, Enter или A — нажать", "Влево/вправо — слайдеры и выбор значений",
        "Колесо — прокрутка списков и слайдер под мышью", "Клик по Мите — искры"},
       {"Esc or B: back; arrows: choose, Enter or A: press", "Left/Right: sliders and choices",
        "Wheel: scrolls the lists and the slider under the mouse", "Click on Mita: sparks"}},
    };
    return ENTRIES;
  }

} // namespace

// CREDITS over a scrolling column: each group's name, then its lines wrapped to the column; BACK under it
class MSCreditsScreen : public MSScreen
{
public:
  static MSCreditsScreen* create()
  {
    auto screen = new MSCreditsScreen();
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
    auto en = english();
    if (!this->initScreen(en ? "CREDITS" : "АВТОРЫ"))
      return false;
    auto const& theme = Theme::get();
    auto top = theme.screenTop + fromTop();
    auto bottom = std::min(theme.screenBottom + fromTop(), top - 20);
    CCRect view{theme.screenLeft, bottom, theme.screenWidth, top - bottom};
    m_scroll = MSScrollView::create(view, theme.screenScrollLeft, theme.screenScrollWidth,
                                    theme.achievementsScrollTrackColor, theme.achievementsScrollThumbColor);
    if (!m_scroll)
      return false;
    m_mainLayer->addChild(m_scroll, 10);

    auto content = m_scroll->content();
    auto width = theme.screenWidth;
    auto y = 0.f;
    auto first = true;
    for (auto const& entry : entries())
    {
      if (!first)
        y -= theme.statsGroupGap;
      first = false;
      if (auto name = ms::createText(en ? entry.en : entry.ru, theme.statsGroupScale, theme.statsGroupColor,
                                     theme.titleFont, ms::Shadow{}).node)
      {
        name->setPosition({2, y - theme.statsGroupHeight / 2 - TITLE_RAISE * theme.statsGroupScale});
        content->addChild(name);
      }
      y -= theme.statsGroupHeight;
      for (auto line : en ? entry.linesEn : entry.linesRu)
      {
        if (auto text = ms::createWrapped(line, theme.achievementsTextScale, theme.achievementsTextColor, width - 4))
        {
          text->setPosition({2, y});
          content->addChild(text);
          y -= text->getScaledContentSize().height + LINE_GAP;
        }
      }
    }
    m_scroll->setContentHeight(-y);
    m_scroll->setStep(theme.statsRowHeight * 2);
    return true;
  }

  bool onKey(enumKeyCodes key) override
  {
    auto step = Theme::get().statsRowHeight * 2;
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
};

namespace ms::credits
{
  void open()
  {
    if (auto screen = MSCreditsScreen::create())
      screen->show();
  }
} // namespace ms::credits
