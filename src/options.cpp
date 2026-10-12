#include "options/MSOptions.hpp"

#include "Theme.hpp"
#include "options/MSOptionsData.hpp"
#include "options/MSOptionsText.hpp"
#include "pause/MSPauseColumn.hpp"
#include "ui/MSScreen.hpp"
#include "ui/MSStyle.hpp"

#include <Geode/Geode.hpp>

#include <algorithm>
#include <map>

using namespace geode::prelude;

namespace
{
  // The info's text gets this much smaller at a time until it fits over the bottom row, down to the smallest
  constexpr float INFO_SHRINK = 0.9f;
  constexpr float INFO_SMALLEST = 0.18f;
  // GD's own page names that have the mod's rows after their toggles
  constexpr char const* AUDIO_PAGE = "Audio";
  constexpr char const* OTHER_PAGE = "Other";

  std::string onOff(bool on, bool en) { return on ? (en ? "ON" : "ВКЛ") : (en ? "OFF" : "ВЫКЛ"); }

  bool variable(std::string const& key) { return GameManager::get()->getGameVariable(key.c_str()); }

  // A click on GD's own button for it: its callback (with everything GD does there) and its look. No button: the
  // variable itself
  void press(ms::options::Toggle const& toggle)
  {
    if (toggle.toggler)
    {
      toggle.toggler->activate();
      return;
    }
    if (!toggle.key.empty())
      GameManager::get()->toggleGameVariable(toggle.key.c_str());
  }

  // A row for a toggle of GD's that's a game variable: its value read from the variable every frame
  void addVariable(MSPauseColumn* column, ms::options::Toggle const& toggle, bool en)
  {
    MSPauseColumn::Choice choice;
    choice.value = [key = toggle.key, en] { return onOff(variable(key), en); };
    choice.change = [toggle](int) { press(toggle); };
    column->addChoice(ms::options::label(toggle, en), std::move(choice));
    column->setInfo(ms::options::info(toggle, en));
  }

  // GD's window that drops down (the account, the soundtracks, the support, the saved songs) over everything, as
  // GD's settings add them
  void dropDown(GJDropDownLayer* layer)
  {
    auto scene = CCDirector::get()->getRunningScene();
    if (!layer || !scene)
      return;
    scene->addChild(layer, std::max(scene->getHighestChildZ(), 105));
    layer->showLayer(false);
  }

  std::string quality(std::string const& gd, bool en)
  {
    if (en)
      return ms::options::upper(gd);
    if (gd == "Auto")
      return "АВТО";
    if (gd == "Low")
      return "НИЗКОЕ";
    if (gd == "Medium")
      return "СРЕДНЕЕ";
    if (gd == "High")
      return "ВЫСОКОЕ";
    return ms::options::upper(gd);
  }

  // Shown on rows GD has hidden for now (the resolution in full screen)
  constexpr char const* UNAVAILABLE = "—";
} // namespace

// ! --- A settings screen --- !

// A screen of the mod's with a column of GD's settings, longer ones scrolling, and under it what the lit row does.
// The GD windows it works are made for it and never shown; they go with it
class MSOptionsScreen : public MSScreen
{
public:
  ~MSOptionsScreen() override
  {
    for (auto const& layer : m_driven)
      ms::options::dispose(layer);
  }

protected:
  bool initOptions(std::string const& title)
  {
    if (!this->initScreen(title))
      return false;
    auto const& theme = Theme::get();
    m_column = MSPauseColumn::create();
    if (!m_column)
      return false;
    m_column->setRowWidth(theme.optionsRowWidth);
    auto top = theme.screenTop + fromTop();
    auto bottom = theme.optionsViewBottom + fromTop();
    m_column->setViewHeight(std::max(top - bottom, theme.pauseRowHeight));
    m_column->setOnInfo([this](std::string const& info) { this->showInfo(info); });
    return true;
  }

  void finishOptions()
  {
    auto const& theme = Theme::get();
    m_mainLayer->addChild(m_column, 10);
    m_column->finish(theme.screenLeft, theme.screenTop + fromTop());
  }

  // GD's window, kept for the rows to work
  template <class T>
  T* drive(T* layer)
  {
    if (layer)
      m_driven.emplace_back(layer);
    return layer;
  }

  // Wrapped to the rows' width under the column, smaller until it fits over the bottom row
  void showInfo(std::string const& text)
  {
    if (text == m_infoText)
      return;
    m_infoText = text;
    if (m_info)
    {
      m_info->removeFromParent();
      m_info = nullptr;
    }
    if (text.empty())
      return;
    auto const& theme = Theme::get();
    auto width = theme.optionsRowWidth - 4;
    auto top = theme.optionsInfoTop + fromTop();
    auto room = std::max(top - (theme.screenBackTop + fromTop()) - 2, 4.f);
    for (auto scale = std::max(theme.optionsInfoScale, INFO_SMALLEST);; scale *= INFO_SHRINK)
    {
      CCNode* node = nullptr;
      if (ms::resourceExists(theme.font))
        node = CCLabelBMFont::create(text.c_str(), theme.font.c_str(), width / scale, kCCTextAlignmentLeft);
      if (!node)
        node = ms::createLabel(text);
      if (!node)
        return;
      node->setScale(scale);
      if (node->getScaledContentSize().height > room && scale * INFO_SHRINK >= INFO_SMALLEST)
        continue;
      node->setAnchorPoint({0, 1});
      node->setPosition({theme.screenLeft + 2, top});
      ms::setColor(node, theme.optionsInfoColor);
      m_mainLayer->addChild(node, 10);
      m_info = node;
      return;
    }
  }

  bool onKey(enumKeyCodes key) override { return m_column && m_column->onKey(key); }

  // Esc while typing a number stops the typing, not the screen
  bool onBack() override { return m_column && m_column->cancelEdit(); }

  MSPauseColumn* m_column = nullptr;
  CCNode* m_info = nullptr;
  std::string m_infoText;
  std::vector<Ref<CCNode>> m_driven;
};

// ! --- The key bindings --- !

// GD's list of keys (it can't change them yet): its groups as names, each action with its keys
class MSKeysScreen : public MSOptionsScreen
{
public:
  static MSKeysScreen* create()
  {
    auto screen = new MSKeysScreen();
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
    if (!this->initOptions(en ? "KEYS" : "КЛАВИШИ"))
      return false;
    this->addBottomButton(en ? "GD WINDOW" : "ОКНО GD", Theme::get().screenButtonWidth, [] {
      if (auto layer = KeybindingsLayer::create())
        layer->show();
    });
    auto layer = this->drive(KeybindingsLayer::create());
    if (!layer)
      return false;
    auto note = en ? std::string("GD can't change its key bindings yet.")
                   : std::string("Клавиши в GD пока нельзя переназначить.");
    ms::options::KeyPair const* last = nullptr;
    for (auto const& pair : ms::options::keyPairs(layer))
    {
      // GD lists one pair twice
      if (last && last->action == pair.action && last->keys == pair.keys)
        continue;
      last = &pair;
      if (pair.keys.empty())
      {
        m_column->addHeader(ms::options::keyAction(pair.action, en));
        continue;
      }
      MSPauseColumn::Choice choice;
      choice.value = [keys = ms::options::keyNames(pair.keys, en)] { return keys; };
      m_column->addChoice(ms::options::keyAction(pair.action, en), std::move(choice));
      m_column->setInfo(note);
    }
    this->finishOptions();
    return true;
  }
};

// ! --- A group of GD's options --- !

// One group of GD's options (Gameplay, Visual...): its toggles, and after them what GD has on its pages besides
class MSOptionsGroupScreen : public MSOptionsScreen
{
public:
  struct Group
  {
    // GD's name for it, and its pages
    std::string name;
    std::vector<int> pages;
    std::vector<ms::options::Toggle> toggles;
  };

  static MSOptionsGroupScreen* create(Ref<MoreOptionsLayer> more, Group const& group, bool audio, bool other)
  {
    auto screen = new MSOptionsGroupScreen();
    if (screen->setup(std::move(more), group, audio, other))
    {
      screen->autorelease();
      return screen;
    }
    delete screen;
    return nullptr;
  }

protected:
  bool setup(Ref<MoreOptionsLayer> more, Group const& group, bool audio, bool other)
  {
    auto en = english();
    if (!this->initOptions(ms::options::category(group.name, en)))
      return false;
    m_more = more;
    auto first = group.pages.empty() ? 0 : group.pages.front();
    this->addBottomButton(en ? "GD WINDOW" : "ОКНО GD", Theme::get().screenButtonWidth, [first] {
      if (auto layer = MoreOptionsLayer::create())
      {
        layer->show();
        layer->goToPage(first);
      }
    });
    for (auto const& toggle : group.toggles)
      addVariable(m_column, toggle, en);
    if (audio)
      this->addAudio(en);
    if (other)
      this->addOther(en);
    this->finishOptions();
    return true;
  }

  // The music offset GD types in, its saved songs and FMOD's state
  void addAudio(bool en)
  {
    auto const& theme = Theme::get();
    MSPauseColumn::Field offset;
    offset.get = [] { return std::to_string(FMODAudioEngine::get()->m_musicOffset); };
    // As GD reads it when its window closes: a whole number
    offset.set = [](std::string const& text) {
      FMODAudioEngine::get()->m_musicOffset = std::atoi(text.c_str());
    };
    offset.step = [step = static_cast<int>(std::max(theme.optionsOffsetStep, 1.f))](int direction) {
      FMODAudioEngine::get()->m_musicOffset += step * direction;
    };
    offset.allowed = "0123456789-";
    offset.maxLength = 6;
    m_column->addField(en ? "MUSIC OFFSET (MS)" : "СМЕЩЕНИЕ МУЗЫКИ (МС)", std::move(offset));
    m_column->setInfo(en ? "Moves the music against the level, in milliseconds. Click or Enter to type it, Left/Right to "
                           "move it."
                         : "Сдвигает музыку относительно уровня, в миллисекундах. Клик или Enter — ввести число, "
                           "влево/вправо — изменить.");
    m_column->addRow(en ? "SAVED SONGS" : "СОХРАНЁННЫЕ ПЕСНИ", [] { dropDown(GJSongBrowser::create()); });
    m_column->setInfo(en ? "The songs saved on this device." : "Песни, сохранённые на этом устройстве.");
    m_column->addRow(en ? "FMOD DEBUG" : "ОТЛАДКА FMOD", [this] {
      if (m_more)
        m_more->onFMODDebug(nullptr);
    });
    m_column->setInfo(en ? "The sound engine's state." : "Состояние звукового движка FMOD.");
  }

  // The key bindings, and the parental control's toggles in place of its window
  void addOther(bool en)
  {
    m_column->addRow(en ? "KEYS" : "КЛАВИШИ", [] {
      if (auto screen = MSKeysScreen::create())
        screen->show();
    });
    m_column->setInfo(en ? "GD's keys in the game and the editor." : "Клавиши GD в игре и в редакторе.");
    auto parental = this->drive(ParentalOptionsLayer::create());
    auto const& toggles = ms::options::toggles(parental);
    if (toggles.empty())
      return;
    m_column->addHeader(en ? "PARENTAL CONTROL" : "РОДИТЕЛЬСКИЙ КОНТРОЛЬ");
    for (auto const& toggle : toggles)
      addVariable(m_column, toggle, en);
  }

  Ref<MoreOptionsLayer> m_more;
};

// ! --- The graphics --- !

// GD's video options and the advanced ones in one column: full screen, borderless and its fix, the resolution and
// the textures' quality with APPLY (GD applies them only then), then GD's advanced toggles and the frame rate
class MSGraphicsScreen : public MSOptionsScreen
{
public:
  static MSGraphicsScreen* create()
  {
    auto screen = new MSGraphicsScreen();
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
    if (!this->initOptions(en ? "GRAPHICS" : "ГРАФИКА"))
      return false;
    this->addBottomButton(en ? "GD WINDOW" : "ОКНО GD", Theme::get().screenButtonWidth, [] {
      if (auto layer = VideoOptionsLayer::create())
        layer->show();
    });
    m_video = this->drive(VideoOptionsLayer::create());
    m_advanced = this->drive(MoreVideoOptionsLayer::create());
    if (!m_video)
      return false;

    m_column->addHeader(en ? "SCREEN" : "ЭКРАН");
    this->addFlag(
      en ? "FULLSCREEN" : "ПОЛНЫЙ ЭКРАН", [](VideoOptionsLayer* video) { video->onFullscreen(nullptr); },
      [](VideoOptionsLayer* video) { return video->m_fullscreen; }, nullptr,
      en ? "Applied with APPLY." : "Применяется кнопкой «ПРИМЕНИТЬ».");
    this->addFlag(
      en ? "BORDERLESS" : "БЕЗ РАМКИ", [](VideoOptionsLayer* video) { video->onBorderless(nullptr); },
      [](VideoOptionsLayer* video) { return video->m_borderless; },
      [](VideoOptionsLayer* video) { return shown(video->m_borderlessToggle); },
      en ? "Full screen in a window without borders. Applied with APPLY."
         : "Полный экран в окне без рамки. Применяется кнопкой «ПРИМЕНИТЬ».");
    this->addFlag(
      en ? "BORDERLESS FIX" : "ИСПРАВЛЕНИЕ БЕЗ РАМКИ", [](VideoOptionsLayer* video) { video->onBorderlessFix(nullptr); },
      [](VideoOptionsLayer* video) { return video->m_fix; },
      [](VideoOptionsLayer* video) { return shown(video->m_fixToggle); },
      en ? "GD's fix for the borderless window. Applied with APPLY."
         : "Исправление GD для окна без рамки. Применяется кнопкой «ПРИМЕНИТЬ».");

    MSPauseColumn::Choice resolution;
    resolution.value = [this] {
      if (!shown(m_video->m_prevResolutionBtn) || !m_video->m_selectedResolutionLabel)
        return std::string(UNAVAILABLE);
      return std::string(m_video->m_selectedResolutionLabel->getString());
    };
    resolution.change = [this](int direction) {
      if (!shown(m_video->m_prevResolutionBtn))
        return;
      if (direction < 0)
        m_video->onResolutionPrev(nullptr);
      else
        m_video->onResolutionNext(nullptr);
    };
    m_column->addChoice(en ? "RESOLUTION" : "РАЗРЕШЕНИЕ", std::move(resolution));
    m_column->setInfo(en ? "The window's size (windowed only). Left/Right choose it; applied with APPLY."
                         : "Размер окна (только в оконном режиме). Влево/вправо — выбрать, применяется кнопкой "
                           "«ПРИМЕНИТЬ».");

    MSPauseColumn::Choice textures;
    textures.value = [this, en] {
      if (!m_video->m_qualityLabel)
        return std::string(UNAVAILABLE);
      return quality(m_video->m_qualityLabel->getString(), en);
    };
    textures.change = [this](int direction) {
      if (direction < 0)
        m_video->onTextureQualityPrev(nullptr);
      else
        m_video->onTextureQualityNext(nullptr);
    };
    m_column->addChoice(en ? "TEXTURE QUALITY" : "КАЧЕСТВО ТЕКСТУР", std::move(textures));
    m_column->setInfo(en ? "Lower takes less memory. Applied with APPLY."
                         : "Чем ниже, тем меньше памяти нужно. Применяется кнопкой «ПРИМЕНИТЬ».");
    m_column->addRow(en ? "APPLY" : "ПРИМЕНИТЬ", [this] { m_video->onApply(nullptr); });
    m_column->setInfo(en ? "Applies the screen's options as GD does (it may reload the game's textures)."
                         : "Применяет настройки экрана, как это делает GD (может перезагрузить текстуры игры).");

    // GD keeps its low detail mode in its support window: its callback there (it doesn't read the button) flips it,
    // lets the menu's background load again and tells what it does the first time
    m_support = this->drive(SupportLayer::create());
    if (m_support)
    {
      MSPauseColumn::Choice lowDetail;
      lowDetail.value = [en] { return onOff(GameManager::get()->m_performanceMode, en); };
      lowDetail.change = [this](int) { m_support->onLowDetail(nullptr); };
      m_column->addChoice(en ? "LOW DETAIL MODE" : "НИЗКАЯ ДЕТАЛИЗАЦИЯ ВЕЗДЕ", std::move(lowDetail));
      m_column->setInfo(en ? "Turns off a lot of visual effects for speed (GD has it in its support window)."
                           : "Отключает многие визуальные эффекты ради скорости (в GD этот переключатель в окне "
                             "«Поддержка»).");
    }

    if (m_advanced)
    {
      m_column->addHeader(en ? "ADVANCED" : "РАСШИРЕННЫЕ");
      for (auto const& toggle : ms::options::toggles(m_advanced.data()))
        addVariable(m_column, toggle, en);
      MSPauseColumn::Field fps;
      fps.get = [this] {
        auto input = m_advanced->m_fpsInput;
        return input ? std::string(input->getString()) : std::string(UNAVAILABLE);
      };
      fps.set = [this](std::string const& text) {
        if (auto input = m_advanced->m_fpsInput)
          input->setString(text);
      };
      fps.allowed = "0123456789.";
      fps.maxLength = 7;
      m_column->addField(en ? "FPS" : "КАДРОВ В СЕКУНДУ", std::move(fps));
      m_column->setInfo(en ? "The target frame rate with UNLOCK FPS on. Click or Enter to type it, then APPLY FPS."
                           : "Предел кадров в секунду при включённом «СВОЙ ПРЕДЕЛ FPS». Клик или Enter — ввести "
                             "число, затем «ПРИМЕНИТЬ FPS».");
      m_column->addRow(en ? "APPLY FPS" : "ПРИМЕНИТЬ FPS", [this] { m_advanced->onApplyFPS(nullptr); });
      m_column->setInfo(en ? "Sets the frame rate typed above, as GD's Apply does."
                           : "Устанавливает введённый предел кадров, как кнопка Apply в GD.");
    }
    this->finishOptions();
    return true;
  }

  static bool shown(CCNode* node)
  {
    if (!node || !node->isVisible())
      return false;
    if (auto item = typeinfo_cast<CCMenuItem*>(node))
      return item->isEnabled();
    return true;
  }

  // A flag GD keeps until APPLY, flipped by the callback of its toggle button (it doesn't read the button);
  // unavailable while GD hides that button
  void addFlag(std::string const& text, void (*flip)(VideoOptionsLayer*), bool (*value)(VideoOptionsLayer*),
               bool (*available)(VideoOptionsLayer*), std::string const& info)
  {
    auto en = english();
    MSPauseColumn::Choice choice;
    choice.value = [this, value, available, en] {
      if (available && !available(m_video))
        return std::string(UNAVAILABLE);
      return onOff(value(m_video), en);
    };
    choice.change = [this, flip, available](int) {
      if (available && !available(m_video))
        return;
      flip(m_video);
    };
    m_column->addChoice(text, std::move(choice));
    m_column->setInfo(info);
  }

  Ref<VideoOptionsLayer> m_video;
  Ref<MoreVideoOptionsLayer> m_advanced;
  Ref<SupportLayer> m_support;
};

// ! --- GD's settings --- !

// The volumes and the menu music, GD's options by group, the graphics, then GD's other windows
class MSGDOptionsScreen : public MSOptionsScreen
{
public:
  static MSGDOptionsScreen* create(std::function<void()> vanilla)
  {
    auto screen = new MSGDOptionsScreen();
    if (screen->setup(std::move(vanilla)))
    {
      screen->autorelease();
      return screen;
    }
    delete screen;
    return nullptr;
  }

protected:
  bool setup(std::function<void()> vanilla)
  {
    auto en = english();
    if (!this->initOptions(en ? "SETTINGS" : "НАСТРОЙКИ"))
      return false;
    m_vanilla = std::move(vanilla);
    this->addBottomButton(en ? "GD WINDOW" : "ОКНО GD", Theme::get().screenButtonWidth,
                          [this] { this->openVanilla(); });
    m_options = this->drive(OptionsLayer::create());
    m_more = this->drive(MoreOptionsLayer::create());
    if (!m_options || !m_more)
      return false;

    m_column->addHeader(en ? "SOUND" : "ЗВУК");
    this->addVolume(true, en ? "MUSIC" : "МУЗЫКА");
    this->addVolume(false, en ? "SFX" : "ЗВУКИ");
    MSPauseColumn::Choice menuMusic;
    // GD's variable is the other way round: on, the menu is quiet
    menuMusic.value = [en] { return onOff(!variable("0122"), en); };
    // GD's callback for its toggle (it doesn't read the button): the variable, and the menu's music stopped or started
    menuMusic.change = [this](int) { m_options->onMenuMusic(nullptr); };
    m_column->addChoice(en ? "MENU MUSIC" : "МУЗЫКА В МЕНЮ", std::move(menuMusic));
    m_column->setInfo(en ? "Music in the game's menus." : "Музыка в меню игры.");

    m_column->addHeader(en ? "OPTIONS" : "ПАРАМЕТРЫ");
    this->groupOptions();
    auto audio = this->findGroup(AUDIO_PAGE);
    auto other = this->findGroup(OTHER_PAGE);
    if (other < 0)
      other = static_cast<int>(m_groups.size()) - 1;
    for (int i = 0; i < static_cast<int>(m_groups.size()); i++)
    {
      m_column->addRow(ms::options::category(m_groups[i].name, en), [this, i, audio, other] {
        if (auto screen = MSOptionsGroupScreen::create(m_more, m_groups[i], i == audio, i == other))
          screen->show();
      });
      m_column->setInfo(this->groupInfo(m_groups[i], en));
    }
    m_column->addRow(en ? "GRAPHICS" : "ГРАФИКА", [] {
      if (auto screen = MSGraphicsScreen::create())
        screen->show();
    });
    m_column->setInfo(en ? "Full screen, resolution, textures, vertical sync and the frame rate."
                         : "Полный экран, разрешение, текстуры, вертикальная синхронизация и частота кадров.");

    m_column->addHeader(en ? "MORE" : "ЕЩЁ");
    m_column->addRow(en ? "ACCOUNT" : "АККАУНТ", [] { dropDown(AccountLayer::create()); });
    m_column->setInfo(en ? "Sign in, save and load your data." : "Вход, сохранение и загрузка данных.");
    m_column->addRow(en ? "SOUNDTRACKS" : "САУНДТРЕКИ", [] { dropDown(SongsLayer::create()); });
    m_column->setInfo(en ? "The official levels' music." : "Музыка официальных уровней.");
    m_column->addRow(en ? "HOW TO PLAY" : "КАК ИГРАТЬ", [this] { m_options->onHelp(nullptr); });
    m_column->setInfo(en ? "GD's tutorial." : "Обучение GD.");
    m_column->addRow(en ? "SUPPORT" : "ПОДДЕРЖКА", [] { dropDown(SupportLayer::create()); });
    m_column->setInfo(en ? "Help, privacy and terms of RobTop Games." : "Помощь, конфиденциальность и условия RobTop "
                                                                        "Games.");
    m_column->addRow(en ? "RATE THE GAME" : "ОЦЕНИТЬ ИГРУ", [this] { m_options->onRate(nullptr); });
    m_column->setInfo(en ? "GD's page in Steam." : "Страница GD в Steam.");
    MSPauseColumn::Choice vault;
    vault.value = [en] {
      auto open = GameStatsManager::sharedState()->getStat("12") >= 10;
      return std::string(open ? (en ? "OPEN" : "ОТКРЫТО") : (en ? "LOCKED" : "ЗАКРЫТО"));
    };
    vault.change = [this](int) { this->openVault(); };
    m_column->addChoice(en ? "VAULT" : "ХРАНИЛИЩЕ", std::move(vault));
    m_column->setInfo(en ? "It's a secret..." : "Это секрет...");
    this->finishOptions();
    return true;
  }

  void addVolume(bool music, std::string const& text)
  {
    Ref<Slider> slider = music ? m_options->m_musicSlider : m_options->m_sfxSlider;
    MSPauseColumn::Slider row;
    row.get = [music] {
      auto engine = FMODAudioEngine::get();
      return music ? engine->m_musicVolume : engine->m_sfxVolume;
    };
    // GD's slider moved and its callback run, as dragging it does
    row.set = [this, slider, music](float value) {
      if (!slider || !slider->getThumb())
        return;
      slider->setValue(value);
      if (music)
        m_options->musicSliderChanged(slider->getThumb());
      else
        m_options->sfxSliderChanged(slider->getThumb());
    };
    m_column->addSlider(text, std::move(row));
  }

  // GD's toggles by the page's name GD shows: pages with the same name are one group
  void groupOptions()
  {
    std::map<int, std::vector<ms::options::Toggle const*>> pages;
    for (auto const& toggle : ms::options::toggles(m_more.data()))
      pages[toggle.page].push_back(&toggle);
    for (auto const& [page, toggles] : pages)
    {
      m_more->goToPage(page);
      std::string name = m_more->m_categoryLabel ? m_more->m_categoryLabel->getString() : "";
      if (name.empty())
        name = "Options";
      auto group = std::find_if(m_groups.begin(), m_groups.end(), [&](auto const& g) { return g.name == name; });
      if (group == m_groups.end())
      {
        m_groups.push_back({name, {}, {}});
        group = m_groups.end() - 1;
      }
      group->pages.push_back(page);
      for (auto toggle : toggles)
        group->toggles.push_back(*toggle);
    }
    if (!pages.empty())
      m_more->goToPage(pages.begin()->first);
  }

  int findGroup(std::string const& name) const
  {
    for (int i = 0; i < static_cast<int>(m_groups.size()); i++)
    {
      if (m_groups[i].name == name)
        return i;
    }
    return -1;
  }

  // The first few toggles of the group, as a hint of what's in it
  std::string groupInfo(MSOptionsGroupScreen::Group const& group, bool en) const
  {
    constexpr size_t SHOWN = 3;
    std::string out;
    for (size_t i = 0; i < group.toggles.size(); i++)
    {
      if (i == SHOWN)
      {
        out += "...";
        break;
      }
      if (!out.empty())
        out += ", ";
      out += ms::options::label(group.toggles[i], en);
    }
    return out;
  }

  // What GD's lock does: the vault (another scene) with 10 secret coins, else its guard's words, which GD puts in
  // its settings window: moved over this screen, the way they came in
  void openVault()
  {
    m_options->onSecretVault(nullptr);
    std::vector<Ref<DialogLayer>> dialogs;
    for (auto child : CCArrayExt<CCNode*>(m_options->getChildren()))
    {
      if (auto dialog = typeinfo_cast<DialogLayer*>(child))
        dialogs.emplace_back(dialog);
    }
    for (auto const& dialog : dialogs)
    {
      dialog->removeFromParentAndCleanup(false);
      this->addChild(dialog, 100);
    }
  }

  // GD's own settings window, with what other mods put in it. GD puts it in the menu (MenuLayer::openOptions), which
  // hides under this screen: the screen goes first, then the menu's button opens it as it always did
  void openVanilla()
  {
    if (!m_vanilla)
      return dropDown(OptionsLayer::create());
    this->close(m_vanilla);
  }

  std::function<void()> m_vanilla;
  Ref<OptionsLayer> m_options;
  Ref<MoreOptionsLayer> m_more;
  std::vector<MSOptionsGroupScreen::Group> m_groups;
};

namespace ms::options
{
  void open(std::function<void()> vanilla, bool graphics)
  {
    if (auto screen = MSGDOptionsScreen::create(vanilla))
    {
      screen->show();
      if (graphics)
      {
        if (auto over = MSGraphicsScreen::create())
          over->show();
      }
      return;
    }
    // Not made: GD's own window, as before
    log::warn("The settings screen couldn't be made: GD's settings open instead");
    if (vanilla)
      vanilla();
  }
} // namespace ms::options
