#include "MSOptions.hpp"
#include "MSOptionsScreen.hpp"
#include "MSWindows.hpp"

#include "../pause/MSPauseColumn.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/ui/OverlayManager.hpp>

#include <deque>
#include <filesystem>
#include <unordered_set>

using namespace geode::prelude;

// The settings screens' self-test: with a file named "selftest" in the mod's save folder (deleted as it starts), the
// game opens every settings screen one after another, each logged with its rows and what they show (SELFTEST in the
// log): the menu's, a level's options and the profile settings, then in the first official level the pause's
// settings and its two UI editors, then the editor's options on a new level that is never saved; then it saves and
// quits as the menu's quit does. Only the rows that open another screen are pressed: no setting is changed
namespace
{
  // Long enough for a screen to come in, its window to load
  constexpr float STEP_TIME = 1.2f;
  constexpr float START_TIME = 4;
  constexpr int DEPTH = 2;

  // The rows that open a screen: GD's groups, the graphics, the account, the soundtracks, how to play, the support;
  // under them the keys, the saved songs, the account's More, the support's link lists
  std::unordered_set<std::string> const& openers()
  {
    static std::unordered_set<std::string> const SET = {
      "ИГРА", "ВИЗУАЛ", "ПРАКТИКА", "ПРОИЗВОДИТЕЛЬНОСТЬ", "ЗВУК", "ПРОЧЕЕ", "ГРАФИКА", "АККАУНТ", "САУНДТРЕКИ",
      "КАК ИГРАТЬ", "ПОДДЕРЖКА", "КЛАВИШИ", "СОХРАНЁННЫЕ ПЕСНИ", "ЕЩЁ", "ССЫЛКИ", "ЗВУКОВЫЕ ЭФФЕКТЫ",
      "GAMEPLAY", "VISUAL", "PRACTICE", "PERFORMANCE", "AUDIO", "OTHER", "GRAPHICS", "ACCOUNT", "SOUNDTRACKS",
      "HOW TO PLAY", "SUPPORT", "KEYS", "SAVED SONGS", "MORE", "LINKS", "SOUND EFFECTS",
    };
    return SET;
  }

  MSOptionsScreen* topScreen()
  {
    auto scene = CCDirector::get()->getRunningScene();
    if (!scene)
      return nullptr;
    MSOptionsScreen* top = nullptr;
    for (auto child : CCArrayExt<CCNode*>(scene->getChildren()))
    {
      if (auto screen = typeinfo_cast<MSOptionsScreen*>(child); screen && !screen->closing())
        top = screen;
    }
    return top;
  }
} // namespace

class MSSelfTest : public CCNode
{
public:
  static MSSelfTest* create(MenuLayer* menu)
  {
    auto node = new MSSelfTest();
    node->init();
    node->autorelease();
    node->m_menu = menu;
    node->m_wait = START_TIME;
    node->plan();
    node->scheduleUpdate();
    return node;
  }

  void update(float dt) override
  {
    m_wait -= dt;
    if (m_wait > 0 || m_steps.empty())
      return;
    auto step = std::move(m_steps.front());
    m_steps.pop_front();
    m_wait = STEP_TIME;
    step();
  }

protected:
  using Step = std::function<void()>;

  void plan()
  {
    log::info("SELFTEST start");
    ms::options::setSelfTesting(true);
    m_steps.push_back([this] { ms::options::open(nullptr); });
    m_steps.push_back(this->explore(0, "SETTINGS"));
    m_steps.push_back([this] { this->back(); });
    m_steps.push_back([] {
      if (auto level = GameLevelManager::get()->getMainLevel(1, false))
        ms::options::openLevelOptions(level);
    });
    m_steps.push_back(this->explore(DEPTH, "LEVEL OPTIONS"));
    m_steps.push_back([this] { this->back(); });
    m_steps.push_back([] {
      if (auto layer = GJAccountSettingsLayer::create(GJAccountManager::get()->m_accountID))
        layer->show();
    });
    // The form comes over GD's window the frame after
    m_steps.push_back([] {});
    m_steps.push_back(this->explore(DEPTH, "PROFILE SETTINGS"));
    m_steps.push_back([this] { this->back(); });

    // In a level: the pause's settings and its UI editors
    m_steps.push_back([] {
      if (auto level = GameLevelManager::get()->getMainLevel(1, false))
        CCDirector::get()->replaceScene(PlayLayer::scene(level, false, false));
    });
    m_steps.push_back([] {});
    m_steps.push_back([] {});
    m_steps.push_back([] {
      if (auto play = PlayLayer::get())
        play->pauseGame(false);
    });
    m_steps.push_back([this] { this->pauseColumn("pause-column", "PAUSE", {"НАСТРОЙКИ", "SETTINGS"}); });
    m_steps.push_back([this] { this->pauseColumn("pause-settings", "PAUSE SETTINGS", {"ИНТЕРФЕЙС ПЛАТФОРМЕРА", "PLATFORMER UI"}); });
    m_steps.push_back([] {});
    m_steps.push_back(this->explore(DEPTH, "PLATFORMER UI"));
    m_steps.push_back([this] { this->back(); });
    m_steps.push_back([this] { this->pauseColumn("pause-settings", "PAUSE SETTINGS", {"ИНТЕРФЕЙС ПРАКТИКИ", "PRACTICE UI"}); });
    m_steps.push_back([] {});
    m_steps.push_back(this->explore(DEPTH, "PRACTICE UI"));
    m_steps.push_back([this] { this->back(); });
    m_steps.push_back([] {
      if (auto play = PlayLayer::get())
        play->onQuit();
    });
    m_steps.push_back([] {});
    m_steps.push_back([] {});

    // In the editor, on a new level that is never saved: its options
    m_steps.push_back([] {
      auto level = GJGameLevel::create();
      level->m_levelType = GJLevelType::Editor;
      level->m_levelName = "MiSide self-test";
      CCDirector::get()->replaceScene(LevelEditorLayer::scene(level, false));
    });
    m_steps.push_back([] {});
    m_steps.push_back([] {});
    m_steps.push_back([] {
      if (auto editor = LevelEditorLayer::get())
      {
        if (auto pause = EditorPauseLayer::create(editor))
          pause->onOptions(nullptr);
      }
    });
    m_steps.push_back(this->explore(DEPTH, "EDITOR OPTIONS"));
    m_steps.push_back([this] { this->back(); });
    m_steps.push_back([] { CCDirector::get()->replaceScene(MenuLayer::scene(false)); });
    m_steps.push_back([] {});
    m_steps.push_back([] {});
    m_steps.push_back([this] { log::info("SELFTEST DONE: {} screens, {} failures", m_screens, m_failures); });
    m_steps.push_back([this] {
      auto application = CCApplication::sharedApplication();
#ifdef GEODE_IS_WINDOWS
      application->setShutdownCalled(true);
#endif
      application->trySaveGame(true);
      if (m_menu)
        m_menu->endGame();
    });
  }

  void back()
  {
    if (auto top = topScreen())
      top->keyBackClicked();
  }

  // Every choice and slider of a column moved a step and back (the other way first at the end of a slider's travel):
  // its value has to change, then come back. The vault (another scene) and what GD puts up meanwhile (closed) aside
  void exercise(MSPauseColumn* column, std::string const& name, MSOptionsScreen* screen)
  {
    static std::unordered_set<std::string> const SKIP = {"ХРАНИЛИЩЕ", "VAULT"};
    auto texts = column->rowTexts();
    int moved = 0;
    std::vector<std::string> wrong;
    for (size_t i = 0; i < texts.size(); i++)
    {
      if (SKIP.contains(texts[i]))
        continue;
      auto before = column->rowValues()[i];
      // Empty: nothing to show; a dash: GD has it hidden for now (borderless in a window)
      if (before.empty() || before == "—")
        continue;
      // A slider at its top goes down first, and is put back exactly (GD rounds a step's way back)
      auto exact = column->sliderValue(i);
      auto direction = exact && *exact > 0.9f ? -1 : 1;
      if (!column->stepRow(i, direction))
        continue;
      auto changed = column->rowValues()[i];
      column->stepRow(i, -direction);
      if (exact)
        column->setSliderValue(i, *exact);
      auto after = column->rowValues()[i];
      // GD's popups from it (an info the first time, a locked unlocker's words): closed
      while (screen && topScreen() && topScreen() != screen)
        topScreen()->keyBackClicked();
      moved++;
      if (changed == before || after != before)
        wrong.push_back(fmt::format("{} ({} -> {} -> {})", texts[i], before, changed, after));
    }
    if (wrong.empty())
      log::info("SELFTEST {} moved: {} values changed and came back", name, moved);
    else
    {
      m_failures++;
      std::string list;
      for (auto const& entry : wrong)
        list += (list.empty() ? "" : " | ") + entry;
      log::error("SELFTEST FAIL {} moved: {} of {} didn't change or come back: {}", name, wrong.size(), moved, list);
    }
  }

  // A column of the pause logged, and its row with one of these texts pressed
  void pauseColumn(char const* id, std::string const& name, std::vector<std::string> const& press)
  {
    auto scene = CCDirector::get()->getRunningScene();
    PauseLayer* pause = nullptr;
    for (auto child : CCArrayExt<CCNode*>(scene ? scene->getChildren() : nullptr))
    {
      if (auto layer = typeinfo_cast<PauseLayer*>(child))
        pause = layer;
    }
    auto column = pause ? typeinfo_cast<MSPauseColumn*>(pause->getChildByID(Mod::get()->expandSpriteName(id))) : nullptr;
    if (!column)
    {
      m_failures++;
      log::error("SELFTEST FAIL {}: no column", name);
      return;
    }
    m_screens++;
    auto texts = column->rowTexts();
    auto values = column->rowValues();
    std::string rows;
    for (size_t i = 0; i < texts.size(); i++)
      rows += fmt::format("{}{}{}", i ? " | " : "", texts[i], values[i].empty() ? "" : " = " + values[i]);
    log::info("SELFTEST {} ({} rows): {}", name, texts.size(), rows);
    // The pause's own volumes are the level's: only its settings are moved
    if (name == "PAUSE SETTINGS" && !m_pauseMoved)
    {
      m_pauseMoved = true;
      this->exercise(column, name, nullptr);
    }
    for (size_t i = 0; i < texts.size(); i++)
    {
      if (std::find(press.begin(), press.end(), texts[i]) != press.end())
        return column->pressRow(i);
    }
    m_failures++;
    log::error("SELFTEST FAIL {}: no row to press", name);
  }

  // The top screen logged; the rows of it that open screens pressed one by one, each explored and closed
  Step explore(int depth, std::string name)
  {
    return [this, depth, name] {
      auto top = topScreen();
      if (!top || !top->column())
      {
        m_failures++;
        log::error("SELFTEST FAIL {}: no screen", name);
        return;
      }
      m_screens++;
      auto column = top->column();
      auto texts = column->rowTexts();
      auto values = column->rowValues();
      std::string rows;
      for (size_t i = 0; i < texts.size(); i++)
        rows += fmt::format("{}{}{}", i ? " | " : "", texts[i], values[i].empty() ? "" : " = " + values[i]);
      log::info("SELFTEST {} ({} rows): {}", name, texts.size(), rows);
      this->exercise(column, name, top);
      if (depth >= DEPTH)
        return;

      std::vector<Step> next;
      // How to play: its pages
      if (name == "КАК ИГРАТЬ" || name == "HOW TO PLAY")
      {
        for (int page = 2; page <= 5; page++)
        {
          next.push_back([this] {
            if (auto top = topScreen())
              top->column()->pressRow(0);
          });
          next.push_back(this->explore(DEPTH, fmt::format("{} page {}", name, page)));
        }
      }
      // The soundtracks: the first song's window
      std::vector<size_t> open;
      if (name == "САУНДТРЕКИ" || name == "SOUNDTRACKS")
        open.push_back(0);
      for (size_t i = 0; i < texts.size(); i++)
      {
        if (openers().contains(texts[i]))
          open.push_back(i);
      }
      for (auto index : open)
      {
        auto text = index < texts.size() ? texts[index] : "";
        next.push_back([this, index, text] {
          m_before = topScreen();
          if (auto top = topScreen())
            top->column()->pressRow(index);
        });
        next.push_back([this, depth, text] {
          auto now = topScreen();
          auto opened = now && now != m_before;
          m_opened.push_back(opened);
          if (!opened)
          {
            m_failures++;
            log::error("SELFTEST FAIL {}: opened nothing", text);
            return;
          }
          this->explore(depth + 1, text)();
        });
        // Back only from a screen it opened (each level of the walk its own)
        next.push_back([this] {
          auto opened = !m_opened.empty() && m_opened.back();
          if (!m_opened.empty())
            m_opened.pop_back();
          if (opened)
            this->back();
        });
      }
      m_steps.insert(m_steps.begin(), next.begin(), next.end());
    };
  }

  Ref<MenuLayer> m_menu;
  std::deque<Step> m_steps;
  float m_wait = 0;
  MSOptionsScreen* m_before = nullptr;
  std::vector<bool> m_opened;
  bool m_pauseMoved = false;
  int m_screens = 0;
  int m_failures = 0;
};

class $modify(MSSelfTestMenu, MenuLayer)
{
  bool init()
  {
    if (!MenuLayer::init())
      return false;
    auto flag = Mod::get()->getSaveDir() / "selftest";
    std::error_code error;
    if (std::filesystem::exists(flag, error))
    {
      std::filesystem::remove(flag, error);
      // Kept across the level and the editor's scenes
      OverlayManager::get()->addChild(MSSelfTest::create(this));
    }
    return true;
  }
};
