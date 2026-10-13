#include "settings/MSSettings.hpp"

#include "credits/MSCredits.hpp"

#include "Theme.hpp"
#include "pause/MSPauseColumn.hpp"
#include "ui/MSConfirm.hpp"
#include "ui/MSScreen.hpp"
#include "ui/MSStyle.hpp"

#include <Geode/Geode.hpp>

#include <cmath>

using namespace geode::prelude;

namespace
{
  // The mod's own settings this page shows (mod.json), the ones a reset puts back
  constexpr char const* KEYS[] = {"menu-language", "show-title", "show-intro", "confirm-level-exit", "parallax-strength",
                                  "vanilla-menu"};
  // The title font's capitals' height at scale 1, and the gap from the confirmation's title to its first button
  constexpr float TITLE_CAP = 17.5f;
  constexpr float CONFIRM_GAP = 5.1f;
  // The parallax setting's range (mod.json) and the step it's set by
  constexpr float PARALLAX_MAX = 2;
  constexpr float PARALLAX_STEP = 0.1f;

  struct Snapshot
  {
    std::string language;
    bool title = false;
    bool intro = false;
    bool confirmExit = false;
    double parallax = 0;
    bool vanilla = false;

    static Snapshot now()
    {
      auto mod = Mod::get();
      return {mod->getSettingValue<std::string>("menu-language"), mod->getSettingValue<bool>("show-title"),
              mod->getSettingValue<bool>("show-intro"),          mod->getSettingValue<bool>("confirm-level-exit"),
              mod->getSettingValue<double>("parallax-strength"), mod->getSettingValue<bool>("vanilla-menu")};
    }

    // What the menu shows when it's made: the language, the logo, the parallax, the vanilla menu
    bool menuDiffers(Snapshot const& other) const
    {
      return language != other.language || title != other.title || std::abs(parallax - other.parallax) > 0.001 ||
             vanilla != other.vanilla;
    }
  };

  // A yes / no setting as a choice row: a click or Left/Right flips it
  MSPauseColumn::Choice toggle(char const* key, bool en, char const* onRu = "ВКЛ", char const* offRu = "ВЫКЛ",
                               char const* onEn = "ON", char const* offEn = "OFF")
  {
    MSPauseColumn::Choice choice;
    choice.value = [=] { return Mod::get()->getSettingValue<bool>(key) ? (en ? onEn : onRu) : (en ? offEn : offRu); };
    choice.change = [key](int) { Mod::get()->setSettingValue<bool>(key, !Mod::get()->getSettingValue<bool>(key)); };
    return choice;
  }
} // namespace

// The appearance settings as a screen of MiSide's: APPEARANCE over a column in groups (the menu, the level, the
// sound), each row with its value: choices flip with a click or Left/Right, sliders drag or move with Left/Right and
// show their number; RESET puts the mod's settings back to their defaults. Leaving it remakes the menu when something
// it shows changed (the language, the logo, the parallax, the vanilla menu)
class MSSettingsScreen : public MSScreen
{
public:
  static MSSettingsScreen* create()
  {
    auto screen = new MSSettingsScreen();
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
    if (!this->initScreen(en ? "APPEARANCE" : "ОФОРМЛЕНИЕ"))
      return false;
    m_opened = Snapshot::now();
    auto const& theme = Theme::get();
    // Who made what the mod uses, and its controls, over this screen
    this->addBottomButton(en ? "CREDITS" : "АВТОРЫ", theme.screenButtonWidth, [] { ms::credits::open(); });
    auto column = MSPauseColumn::create();
    if (!column)
      return false;

    column->addHeader(en ? "MENU" : "МЕНЮ");
    MSPauseColumn::Choice language;
    language.value = [en] {
      auto ru = Mod::get()->getSettingValue<std::string>("menu-language") != "EN";
      return std::string(ru ? (en ? "RUSSIAN" : "РУССКИЙ") : (en ? "ENGLISH" : "АНГЛИЙСКИЙ"));
    };
    language.change = [](int) {
      auto ru = Mod::get()->getSettingValue<std::string>("menu-language") != "EN";
      Mod::get()->setSettingValue<std::string>("menu-language", ru ? "EN" : "RU");
    };
    column->addChoice(en ? "LANGUAGE" : "ЯЗЫК", std::move(language));
    column->addChoice(en ? "LOGO" : "ЛОГОТИП", toggle("show-title", en));
    column->addChoice(en ? "STUDIO INTRO" : "ИНТРО", toggle("show-intro", en));
    MSPauseColumn::Slider parallax;
    parallax.get = [] {
      return static_cast<float>(Mod::get()->getSettingValue<double>("parallax-strength")) / PARALLAX_MAX;
    };
    parallax.set = [](float value) {
      auto strength = std::round(value * PARALLAX_MAX / PARALLAX_STEP) * PARALLAX_STEP;
      Mod::get()->setSettingValue<double>("parallax-strength", std::clamp(strength, 0.f, PARALLAX_MAX));
    };
    parallax.reset = 1 / PARALLAX_MAX;
    parallax.format = [](float value) { return fmt::format("{:.1f}", value * PARALLAX_MAX); };
    column->addSlider(en ? "PARALLAX" : "ПАРАЛЛАКС", std::move(parallax));
    column->addChoice(en ? "VANILLA MENU" : "МЕНЮ GD", toggle("vanilla-menu", en));

    column->addHeader(en ? "LEVEL" : "УРОВЕНЬ");
    column->addChoice(en ? "LEAVING" : "ВЫХОД",
                      toggle("confirm-level-exit", en, "СПРОСИТЬ", "СРАЗУ", "ASK FIRST", "AT ONCE"));

    // GD's own volumes, as its settings set them (the mod's sounds follow the effects')
    column->addHeader(en ? "SOUND" : "ЗВУК");
    for (auto music : {true, false})
    {
      MSPauseColumn::Slider volume;
      volume.get = [music] {
        auto engine = FMODAudioEngine::get();
        return music ? engine->m_musicVolume : engine->m_sfxVolume;
      };
      volume.set = [music](float value) {
        auto engine = FMODAudioEngine::get();
        auto manager = GameManager::get();
        if (music)
        {
          engine->setBackgroundMusicVolume(value);
          manager->m_bgVolume = value;
        }
        else
        {
          engine->setEffectsVolume(value);
          manager->m_sfxVolume = value;
        }
      };
      column->addSlider(music ? (en ? "MUSIC" : "МУЗЫКА") : (en ? "EFFECTS" : "ЗВУКИ"), std::move(volume));
    }

    column->addRow(en ? "RESET" : "СБРОСИТЬ", [this] { this->openConfirm(); });
    // Down to the bottom row, scrolling past it
    column->setViewHeight(theme.screenTop - theme.screenBottom - 4);
    m_mainLayer->addChild(column, 10);
    column->finish(theme.screenLeft, theme.screenTop + fromTop());
    m_column = column;

    // RESET asks first, in the column's place, as quitting a level does in the pause
    MSConfirm::Texts texts{en ? "RESET THE SETTINGS?" : "СБРОСИТЬ НАСТРОЙКИ?", en ? "YES" : "ДА", en ? "NO" : "НЕТ"};
    auto capTop = theme.screenTop + fromTop();
    MSConfirm::Layout layout{theme.screenLeft,
                             capTop,
                             theme.confirmTitleScale,
                             theme.screenLeft,
                             capTop - TITLE_CAP * theme.confirmTitleScale - CONFIRM_GAP,
                             theme.pauseRowPitch,
                             theme.pauseRowWidth};
    m_confirm = MSConfirm::create(
      texts,
      [this] {
        for (auto key : KEYS)
        {
          if (auto setting = Mod::get()->getSetting(key))
            setting->reset();
        }
        this->closeConfirm();
      },
      [this] { this->closeConfirm(); }, layout, MSMenuItem::Style::pause());
    if (m_confirm)
      m_mainLayer->addChild(m_confirm, 11);
    return true;
  }

  void openConfirm()
  {
    // None (its text couldn't be made): reset at once, as before
    if (!m_confirm)
    {
      for (auto key : KEYS)
      {
        if (auto setting = Mod::get()->getSetting(key))
          setting->reset();
      }
      return;
    }
    if (m_confirm->isOpen())
      return;
    m_column->setShown(false);
    this->setBottomEnabled(false);
    m_confirm->open();
  }

  void closeConfirm()
  {
    if (!m_confirm || !m_confirm->isOpen())
      return;
    m_confirm->close();
    m_column->setShown(true);
    this->setBottomEnabled(true);
  }

  // Open, the confirmation takes every key, Esc closing it
  bool onKey(enumKeyCodes key) override
  {
    if (m_confirm && m_confirm->isOpen())
    {
      m_confirm->onKey(key);
      return true;
    }
    return m_column && m_column->onKey(key);
  }

  bool onBack() override
  {
    if (!m_confirm || !m_confirm->isOpen())
      return false;
    m_confirm->keyBack();
    return true;
  }

  // The menu made again when what it shows changed, through GD's own fade
  void closed() override
  {
    if (!m_opened.menuDiffers(Snapshot::now()))
      return;
    log::info("Appearance changed: the menu is made again");
    CCDirector::get()->replaceScene(CCTransitionFade::create(0.5f, MenuLayer::scene(false)));
  }

  ms::wave::Steps contentSteps() override { return m_column->pageSteps(); }

  MSPauseColumn* m_column = nullptr;
  MSConfirm* m_confirm = nullptr;
  Snapshot m_opened;
};

namespace ms::settings
{
  void open()
  {
    if (auto screen = MSSettingsScreen::create())
      screen->show();
  }
} // namespace ms::settings
