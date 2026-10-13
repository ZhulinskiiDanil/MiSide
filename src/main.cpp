#include "Theme.hpp"
#include "options/MSOptions.hpp"
#include "settings/MSSettings.hpp"
#include "ui/MSBackground.hpp"
#include "ui/MSConfirm.hpp"
#include "ui/MSLayoutGuard.hpp"
#include "ui/MSLogo.hpp"
#include "ui/MSMita.hpp"
#include "ui/MSMenuColumn.hpp"
#include "ui/MSModTray.hpp"
#include "ui/MSMorePopup.hpp"
#include "ui/MSProfileCard.hpp"
#include "ui/MSScenes.hpp"
#include "ui/MSStyle.hpp"
#include "ui/MSWave.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

#include <limits>

using namespace geode::prelude;

namespace
{
  bool s_greetingShown = false;
  // A node by its ID, from Geode's IDs for MenuLayer (loader/src/ids/MenuLayer.cpp). Missing: a warning,
  // and the step that needed it is skipped
  CCNode *find(CCNode *parent, std::string_view id, bool required = true)
  {
    auto node = parent ? parent->getChildByID(id) : nullptr;
    if (!node && required)
      log::warn("Node {} not found, skipped", id);
    return node;
  }

  // A vanilla button in its menu, or wherever another mod moved it
  CCMenuItem *findButton(CCNode *layer, std::string_view menu, std::string_view id)
  {
    auto node = find(find(layer, menu, false), id, false);
    if (!node)
      node = layer->getChildByIDRecursive(id);
    auto button = typeinfo_cast<CCMenuItem *>(node);
    if (!button)
      log::warn("Button {} not found, skipped", id);
    return button;
  }

  struct Entry
  {
    char const *ru;
    char const *en;
    // Where Geode's IDs put the vanilla button
    char const *menu;
    char const *button;
    // Quit: apart from the rows, near the bottom
    bool exit = false;
  };

  // Item -> the vanilla button it presses
  constexpr Entry ENTRIES[] = {
      {"ИГРАТЬ", "PLAY", "main-menu", "play-button"},
      {"ГАРДЕРОБ", "WARDROBE", "main-menu", "icon-kit-button"},
      {"СОЗДАТЬ", "CREATE", "main-menu", "editor-button"},
      {"НАСТРОЙКИ", "SETTINGS", "bottom-menu", "settings-button"},
      {"ДОСТИЖЕНИЯ", "ACHIEVEMENTS", "bottom-menu", "achievements-button"},
      {"СТАТИСТИКА", "STATISTICS", "bottom-menu", "stats-button"},
      {"ВЫХОД", "EXIT", "close-menu", "close-button", true},
  };

  // The menus whose buttons go in the tray's row, in this order: the ones the other mods add theirs to, with the
  // vanilla buttons that do something and have no item (the daily chest, Newgrounds). profile-menu's own button
  // is the profile card; close-menu and top-right-menu stay at the top
  char const *const TRAY_SOURCES[] = {"bottom-menu", "right-side-menu", "profile-menu", "main-menu", "side-menu"};

  // The menus whose buttons are in the "…" popup instead: RobTop, the social links, More Games. Hidden as a whole,
  // their buttons stay in them
  char const *const POPUP_SOURCES[] = {"social-media-menu", "more-games-menu"};

  // Behind everything of the menu
  constexpr int BACKGROUND_Z = -100;
  // Under the vanilla menus (the tray's slots go under their buttons)
  constexpr int TRAY_Z = -1;
  // Above the vanilla menus, below the transition
  constexpr int COLUMN_Z = 10;
  // The quit confirmation, over the column
  constexpr int CONFIRM_Z = 11;
  // GD's own delay between saving and ending (MenuLayer::FLAlert_Clicked, the quit popup's Yes)
  constexpr float QUIT_DELAY = 0.5f;
  // Over the whole menu

  // Hidden, never removed: the node stays in the tree for the mods that look for it
  void hide(CCNode *node)
  {
    if (node)
      node->setVisible(false);
  }
} // namespace

class $modify(MSMenuLayer, MenuLayer)
{
  static void onModify(auto &self)
  {
    // After the other mods' init: the buttons they add are already there
    if (self.setHookPriorityPost("MenuLayer::init", Priority::VeryLate).isErr())
      log::warn("Can't set the MenuLayer::init hook's priority, buttons added after it are picked up a frame later");
  }

  struct Fields
  {
    MSMenuColumn *m_column = nullptr;
    MSMita *m_mita = nullptr;
    MSProfileCard *m_card = nullptr;
    MSConfirm *m_confirm = nullptr;
    // The confirmation closed on this frame: the same Esc doesn't open it again
    unsigned m_closedOnFrame = 0;
    // GD may send Esc both as a key and as back: the press that opened it doesn't close it
    unsigned m_openedOnFrame = 0;
    bool m_quitting = false;
    // GD's settings button (hidden, the column presses it), and whether GD's own window is asked for
    Ref<CCMenuItem> m_settingsButton;
    bool m_vanillaOptions = false;
    // GD's CREATE button: its screen is a page of the mod's too, gone to as a page
    Ref<CCMenuItem> m_creatorButton;
    bool m_greeting = false;
    bool m_greetingInterface = false;
    float m_greetingClock = 0;
    float m_greetingDuration = 0;
    ms::wave::Steps m_greetingSteps;
    CCNodeRGBA *m_greetingVeil = nullptr;
    CCLayerGradient* m_greetingVeilBase = nullptr;
    CCNode* m_greetingVeilGlow = nullptr;
    CCNode *m_greetingGlow = nullptr;
    CCNode *m_greetingEdges = nullptr;
  };

  bool init()
  {
    if (!MenuLayer::init())
      return false;
    // The vanilla menu: nothing else happens here
    if (Mod::get()->getSettingValue<bool>("vanilla-menu"))
      return true;

    Theme::reload();
    this->hideVanilla();
    this->addBackground();
    this->addColumn();
    this->addConfirm();
    this->addProfileCard();
    this->addTray();
    this->addGuard();
    // Last: her place in the drawing order comes from everything else that's in the layer by now
    this->addMita();
    // The page comes in from the left once the scene is in (from the game's start, a level, CREATE)
    if (!s_greetingShown && m_fields->m_mita)
    {
      s_greetingShown = true;
      m_fields->m_greeting = true;
      m_fields->m_greetingSteps = this->pageSteps(true);
      if (m_fields->m_column)
      {
        m_fields->m_column->setBusy(true);
      }
      ms::wave::hide(m_fields->m_greetingSteps);
      m_fields->m_mita->setVisible(false);
      ms::scenes::whenShown(this, [this]
                            { this->startGreeting(); });
    }
    else
    {
      ms::wave::hide(this->pageSteps());
      ms::scenes::whenShown(this, [this]
                            { ms::wave::in(this->pageSteps()); });
    }
    // The menu's music back (from the icon kit's): GD starts it only when it isn't already playing
    GameManager::get()->fadeInMenuMusic();
    return true;
  }

  void keyDown(enumKeyCodes key, double timestamp)
  {
    if (m_fields->m_greeting)
      return;
    // The confirmation open: every key is its own, none reaches the hidden list or GD's own (a controller's A
    // plays, Y opens the icon kit)
    if (auto confirm = m_fields->m_confirm; confirm && confirm->isOpen())
    {
      if ((key == KEY_Escape || key == CONTROLLER_B) &&
          m_fields->m_openedOnFrame == CCDirector::get()->getTotalFrames())
        return;
      confirm->onKey(key);
      if (!confirm->isOpen())
        m_fields->m_closedOnFrame = CCDirector::get()->getTotalFrames();
      return;
    }
    // Passed on, so the mods hooking keyDown after this one get the keys; but not an Enter the column pressed an
    // item with: GD's own Enter plays, and its transition would win over the item's
    if (auto column = m_fields->m_column; column && column->onKey(key) && key == KEY_Enter)
      return;
    MenuLayer::keyDown(key, timestamp);
  }

  // Esc (and a controller's B): GD's quit popup, or the confirmation in its place; open, it goes back
  void keyBackClicked()
  {
    if (m_fields->m_greeting)
      return;
    auto confirm = m_fields->m_confirm;
    auto column = m_fields->m_column;
    if (!confirm || !column)
      return MenuLayer::keyBackClicked();
    if (confirm->isOpen())
    {
      if (m_fields->m_openedOnFrame != CCDirector::get()->getTotalFrames())
        confirm->keyBack();
      return;
    }
    if (m_fields->m_closedOnFrame == CCDirector::get()->getTotalFrames())
      return;
    this->openConfirm();
  }

  // ! --- Vanilla, hidden --- !

  void hideVanilla()
  {
    // The background with the icons running: hidden, and clicks no longer reach its icons
    if (auto background = find(this, "main-menu-bg"))
    {
      hide(background);
      if (auto layer = typeinfo_cast<CCLayer *>(background))
        layer->setTouchEnabled(false);
    }
    hide(find(this, "main-title"));

    // Only until the button is first clicked, or with a controller
    for (auto id : {"character-select-hint", "level-editor-hint", "play-gamepad-icon", "icon-kit-gamepad-icon",
                    "editor-gamepad-icon"})
      hide(find(this, id, false));

    // Their buttons are in the "…" popup now
    for (auto id : POPUP_SOURCES)
      hide(find(this, id));

    // The buttons with an item in the column (Play, Icon Kit, Editor and the others) are hidden in addColumn(),
    // the profile button and the player's name in addProfileCard()
  }

  // ! --- The background --- !

  void addBackground()
  {
    auto strength = static_cast<float>(Mod::get()->getSettingValue<double>("parallax-strength"));
    auto background = MSBackground::create(strength);
    if (!background)
      return;
    background->setID("background"_spr);
    this->addChild(background, BACKGROUND_Z);
  }

  // ! --- The column --- !

  void addColumn()
  {
    auto column = MSMenuColumn::create([this](MSMenuItem *item)
                                       { this->runItem(item); });
    if (!column)
      return;
    auto english = this->isEnglish();
    if (Mod::get()->getSettingValue<bool>("show-title"))
    {
      column->setLogo(MSLogo::create());
      column->setSubtitle(english ? "MAIN MENU" : "ГЛАВНОЕ МЕНЮ");
    }
    std::vector<CCNode *> menus;
    for (auto const &entry : ENTRIES)
    {
      // No vanilla button, no item; but GD on phones has no quit button (its back key asks): EXIT is there all the
      // same, asking in the column, with a hidden button of the mod's own doing what the back key does
      auto button = findButton(this, entry.menu, entry.button);
      auto own = false;
      if (!button && entry.exit)
      {
        button = CCMenuItemExt::createSpriteExtra(CCNode::create(), [this](auto)
                                                  { this->keyBackClicked(); });
        own = true;
      }
      if (!button)
        continue;
      auto text = english ? entry.en : entry.ru;
      if (entry.exit)
        column->setExit(text, button);
      else
        column->addEntry(text, button);
      // The mod's appearance settings right after GD's: no vanilla button, a hidden one of its own, pressed through
      // the column's darkening as the others are
      if (std::string_view(entry.button) == "editor-button")
        m_fields->m_creatorButton = button;
      if (std::string_view(entry.button) == "settings-button")
      {
        m_fields->m_settingsButton = button;
        auto appearance = CCMenuItemExt::createSpriteExtra(CCNode::create(), [](auto)
                                                           { ms::settings::open(); });
        column->addEntry(english ? "APPEARANCE" : "ОФОРМЛЕНИЕ", appearance);
      }
      if (own)
        continue;
      // The column has it now. An invisible button can't be clicked or laid out, but activate() still runs it
      hide(button);
      if (auto menu = button->getParent(); std::find(menus.begin(), menus.end(), menu) == menus.end())
        menus.push_back(menu);
    }
    // Their layouts close the gaps the hidden buttons left
    for (auto menu : menus)
      menu->updateLayout();
    column->setID("column"_spr);
    this->addChild(column, COLUMN_Z);
    column->finish();
    m_fields->m_column = column;
    column->setOnGazeTarget([this](std::optional<CCPoint> target)
                            {
      if (m_fields->m_mita)
        m_fields->m_mita->setMenuGazeTarget(target); });
    column->setOnExitHover([this](bool on)
                           {
      if (m_fields->m_mita)
        m_fields->m_mita->setExitHovered(on); });
  }

  // MAIN MENU and its rows, then the rest that goes with them (only fading): the tray, the profile card, the mods'
  // nodes. The logo, the background and Mita stay
  ms::wave::Steps pageSteps(bool includeLogo = false)
  {
    auto column = m_fields->m_column;
    if (!column)
      return {};
    auto steps = column->pageSteps(includeLogo);
    ms::wave::Step rest;
    rest.slide = false;
    for (auto child : CCArrayExt<CCNode *>(this->getChildren()))
    {
      auto id = child->getID();
      if (child == column || child == m_fields->m_mita || id == "background"_spr)
        continue;
      if (ms::wave::shows(child))
        rest.nodes.push_back(child);
    }
    if (!rest.nodes.empty())
      steps.push_back(std::move(rest));
    return steps;
  }

  // ! --- Running an item --- !

  // The vanilla button's activate(): its callback, and every mod's hook on it, run as usual. CREATE as a page:
  // the menu's out, then GD's screen at once (no fade through black), coming in the same way
  void runItem(MSMenuItem *item)
  {
    Ref<CCMenuItem> target = item->getTarget();
    auto column = m_fields->m_column;
    if (!target || !column)
      return;
    if (m_fields->m_mita)
      m_fields->m_mita->setExitHovered(false);
    // Quit: the confirmation, in the column, instead of GD's popup
    if (item == column->getExit() && m_fields->m_confirm)
    {
      this->openConfirm();
      return;
    }
    if (target != m_fields->m_creatorButton)
    {
      target->activate();
      return;
    }
    column->setBusy(true);
    auto gone = ms::wave::out(this->pageSteps());
    Ref<MenuLayer> self = this;
    this->runAction(CCSequence::createWithTwoActions(CCDelayTime::create(gone), CallFuncExt::create([self, target]
                                                                                                    {
      auto menu = static_cast<MSMenuLayer*>(self.data());
      ms::scenes::skipNextTransition();
      target->activate();
      // No scene came (a mod's popup instead): the menu back as it was
      if (!ms::scenes::skipped())
      {
        menu->m_fields->m_column->setBusy(false);
        ms::wave::in(menu->pageSteps());
      } })));
  }

  // ! --- GD's settings --- !

  // However GD opens its settings (the settings button through onOptions, a key, the graphics opened again after GD
  // applied them), the mod's screens open in their place; GD WINDOW on them asks for GD's own window
  void openOptions(bool videoOptions)
  {
    if (!m_fields->m_column || m_fields->m_vanillaOptions)
      return MenuLayer::openOptions(videoOptions);
    Ref<MSMenuLayer> self = this;
    ms::options::open([self]
                      { self->openVanillaOptions(); }, videoOptions);
  }

  // GD's own settings window, through the settings button as a click would (the mods' hooks on onOptions run too)
  void openVanillaOptions()
  {
    m_fields->m_vanillaOptions = true;
    if (auto button = m_fields->m_settingsButton)
      button->activate();
    else
      this->onOptions(nullptr);
    m_fields->m_vanillaOptions = false;
  }

  // ! --- Quitting --- !

  // The column's own confirmation instead of GD's popup; with none (its text couldn't be made), GD's popup
  void addConfirm()
  {
    if (!m_fields->m_column)
      return;
    auto english = this->isEnglish();
    MSConfirm::Texts texts{english ? "QUIT THE GAME?" : "ВЫЙТИ ИЗ ИГРЫ?", english ? "YES" : "ДА",
                           english ? "NO" : "НЕТ"};
    auto confirm = MSConfirm::create(texts, [this]
                                     { this->quitGame(); }, [this]
                                     { this->closeConfirm(); });
    if (!confirm)
      return;
    confirm->setID("quit-confirm"_spr);
    this->addChild(confirm, CONFIRM_Z);
    m_fields->m_confirm = confirm;
  }

  void openConfirm()
  {
    auto confirm = m_fields->m_confirm;
    auto column = m_fields->m_column;
    if (!confirm || !column || confirm->isOpen() || m_fields->m_quitting || column->isBusy() || !column->isOnTop())
      return;
    column->setConfirming(true);
    confirm->open();
    if (m_fields->m_mita)
    {
      m_fields->m_mita->setExitHovered(false);
      m_fields->m_mita->setExitConfirmation(true);
    }
    m_fields->m_openedOnFrame = CCDirector::get()->getTotalFrames();
  }

  void closeConfirm()
  {
    auto confirm = m_fields->m_confirm;
    if (!confirm || !confirm->isOpen() || m_fields->m_quitting)
      return;
    confirm->close();
    if (m_fields->m_mita)
      m_fields->m_mita->setExitConfirmation(false);
    m_fields->m_column->setConfirming(false);
    m_fields->m_closedOnFrame = CCDirector::get()->getTotalFrames();
  }

  // What GD's quit popup does on Yes (MenuLayer::FLAlert_Clicked, tag 0): the shutdown flag, the save, and the end
  // half a second later. Once only; no process is killed and no popup is shown again
  void quitGame()
  {
    if (m_fields->m_quitting)
      return;
    m_fields->m_quitting = true;
    log::info("Quitting from the confirmation: saving, then ending as GD's popup does");
    auto application = CCApplication::sharedApplication();
#ifdef GEODE_IS_WINDOWS
    // Windows' GD only: its flag that the game is ending
    application->setShutdownCalled(true);
#endif
    application->trySaveGame(true);
    auto end = CCSequence::createWithTwoActions(CCDelayTime::create(QUIT_DELAY),
                                                CCCallFunc::create(this, callfunc_selector(MenuLayer::endGame)));
    CCDirector::get()->getActionManager()->addAction(end, this, false);
  }

  bool isEnglish() { return Mod::get()->getSettingValue<std::string>("menu-language") == "EN"; }

  // ! --- The profile card --- !

  void addProfileCard()
  {
    auto button = findButton(this, "profile-menu", "profile-button");
    if (!button)
      return;
    auto label = find(this, "player-username", false);
    std::string name = GameManager::get()->m_playerName;
    if (auto text = typeinfo_cast<CCLabelBMFont *>(label))
      name = text->getString();

    auto card = MSProfileCard::create(button, name);
    if (!card)
      return;
    // The card shows them now; the button still opens the profile through activate()
    hide(button);
    hide(label);
    if (auto menu = button->getParent())
      menu->updateLayout();
    card->setID("profile-card"_spr);
    this->addChild(card, COLUMN_Z);
    m_fields->m_card = card;
  }

  // ! --- The tray --- !

  // The buttons the popup shows: what's visible of its menus' buttons
  std::vector<CCMenuItem *> popupButtons()
  {
    std::vector<CCMenuItem *> buttons;
    for (auto id : POPUP_SOURCES)
    {
      auto menu = find(this, id, false);
      if (!menu)
        continue;
      for (auto child : CCArrayExt<CCNode *>(menu->getChildren()))
      {
        if (auto button = typeinfo_cast<CCMenuItem *>(child); button && button->isVisible())
          buttons.push_back(button);
      }
    }
    return buttons;
  }

  void openPopup()
  {
    auto buttons = this->popupButtons();
    if (buttons.empty())
      return;
    if (auto popup = MSMorePopup::create(buttons, this->isEnglish() ? "More" : "Ещё"))
      popup->show();
  }

  void addTray()
  {
    std::function<void()> more;
    if (!this->popupButtons().empty())
      more = [this]
      { this->openPopup(); };
    auto tray = MSModTray::create({std::begin(TRAY_SOURCES), std::end(TRAY_SOURCES)}, std::move(more));
    if (!tray)
      return;
    tray->setID("mod-tray"_spr);
    this->addChild(tray, TRAY_Z);
  }

  // ! --- Nothing over the column --- !

  void addGuard()
  {
    auto column = m_fields->m_column;
    if (!column)
      return;
    std::vector<std::string> skipped{std::begin(TRAY_SOURCES), std::end(TRAY_SOURCES)};
    skipped.insert(skipped.end(), std::begin(POPUP_SOURCES), std::end(POPUP_SOURCES));
    skipped.emplace_back("main-menu-bg");
    auto guard = MSLayoutGuard::create(column, std::move(skipped));
    if (!guard)
      return;
    guard->setID("layout-guard"_spr);
    this->addChild(guard);
  }

  // ! --- Mita --- !

  void startGreeting()
  {
    auto &f = *m_fields.operator->();
    f.m_greetingDuration = f.m_mita->greetingDuration();
    if (f.m_greetingDuration <= 0)
    {
      f.m_greeting = false;
      f.m_mita->setVisible(true);
      if (f.m_column)
        f.m_column->setBusy(false);
      ms::wave::in(f.m_greetingSteps);
      return;
    }
    auto win = CCDirector::get()->getWinSize();
    auto const &theme = Theme::get();
    auto z = f.m_mita->getZOrder();
    f.m_greetingGlow = ms::createStretched(theme.glowSprite, {win.width, win.height * 1.4f}, ccc3(132, 22, 120), 180);
    if (auto glow = f.m_greetingGlow)
    {
      glow->setPosition({win.width * 0.5f, win.height * 0.55f});
      this->addChild(glow, z - 1);
    }
    f.m_greetingEdges = ms::createStretched(theme.vignetteSprite, win, ccc3(12, 2, 18), 210);
    if (auto edges = f.m_greetingEdges)
    {
      edges->setPosition({win.width * 0.5f, win.height * 0.5f});
      this->addChild(edges, z);
    }
    f.m_greetingVeil = CCNodeRGBA::create();
    auto base = CCLayerGradient::create(ccc4(20, 3, 34, 255), ccc4(55, 5, 49, 255), {1, 0});
    f.m_greetingVeilBase = base;
    base->setContentSize(win);
    f.m_greetingVeil->addChild(base);
    if (auto glow = ms::createStretched(theme.glowSprite, {win.width * 1.1f, win.height * 1.6f}, ccc3(140, 15, 145), 190))
    {
      f.m_greetingVeilGlow = glow;
      glow->setPosition({win.width * 0.45f, win.height * 0.5f});
      f.m_greetingVeil->addChild(glow);
    }
    this->addChild(f.m_greetingVeil, COLUMN_Z + 1);
    f.m_mita->setGreeting(greeting::at(0, f.m_greetingDuration));
    f.m_mita->setVisible(true);
    this->schedule(schedule_selector(MSMenuLayer::stepGreeting));
  }

  void stepGreeting(float dt)
  {
    auto &f = *m_fields.operator->();
    f.m_greetingClock += std::clamp(dt, 0.f, 0.1f);
    auto state = greeting::at(f.m_greetingClock, f.m_greetingDuration);
    f.m_mita->setGreeting(state);
    auto win = CCDirector::get()->getWinSize();
    // CCLayerGradient needs its own setOpacity to rebuild the vertex colors;
    // inherited cascade opacity alone leaves the opaque veil drawn over Mita.
    if (f.m_greetingVeilBase)
      f.m_greetingVeilBase->setOpacity(static_cast<GLubyte>(255 * state.veil));
    if (auto rgba = typeinfo_cast<CCRGBAProtocol*>(f.m_greetingVeilGlow))
      rgba->setOpacity(static_cast<GLubyte>(190 * state.veil));
    if (auto glow = f.m_greetingGlow)
    {
      if (auto rgba = typeinfo_cast<CCRGBAProtocol*>(glow))
        rgba->setOpacity(static_cast<GLubyte>(180 * state.aura));
      glow->setPosition({win.width * (0.5f + 0.25f * (1 - state.zoom)), win.height * 0.55f});
    }
    if (auto edges = f.m_greetingEdges)
    {
      if (auto rgba = typeinfo_cast<CCRGBAProtocol*>(edges))
        rgba->setOpacity(static_cast<GLubyte>((150 + 60 * state.zoom) * state.aura));
      auto size = edges->getContentSize();
      edges->setScaleX(win.width / size.width * (1 + 0.12f * (1 - state.zoom)));
      edges->setScaleY(win.height / size.height * (1 + 0.12f * (1 - state.zoom)));
    }
    if (state.showInterface && !f.m_greetingInterface)
    {
      f.m_greetingInterface = true;
      ms::wave::in(f.m_greetingSteps);
    }
    if (state.done)
    {
      f.m_greeting = false;
      if (f.m_column)
        f.m_column->setBusy(false);
      for (CCNode *node : {static_cast<CCNode *>(f.m_greetingVeil), static_cast<CCNode *>(f.m_greetingGlow), static_cast<CCNode *>(f.m_greetingEdges)})
        if (node)
          node->removeFromParentAndCleanup(true);
      f.m_greetingVeil = nullptr;
      f.m_greetingVeilBase = nullptr;
      f.m_greetingVeilGlow = nullptr;
      f.m_greetingGlow = f.m_greetingEdges = nullptr;
      f.m_greetingSteps.clear();
      this->unschedule(schedule_selector(MSMenuLayer::stepGreeting));
    }
  }

  void addMita()
  {
    auto mita = MSMita::carry();
    if (!mita)
      return;
    mita->setID("mita"_spr);
    this->addChild(mita, this->mitaZ());
    m_fields->m_mita = mita;
  }

  // Over the background, under everything else that shows: one under the lowest visible node that isn't the
  // background (the tray, the vanilla menus, the mods' nodes). Hidden vanilla nodes don't count
  int mitaZ()
  {
    auto background = this->getChildByID("background"_spr);
    auto floor = background ? background->getZOrder() : BACKGROUND_Z;
    auto lowest = std::numeric_limits<int>::max();
    for (auto child : CCArrayExt<CCNode *>(this->getChildren()))
    {
      if (child != background && child->isVisible())
        lowest = std::min(lowest, child->getZOrder());
    }
    if (lowest == std::numeric_limits<int>::max())
      return floor + 1;
    if (lowest - 1 <= floor)
    {
      log::warn("A node at z {} is as low as the background: Mita goes right over the background, over it too",
                lowest);
      return floor + 1;
    }
    log::debug("Mita at z {}: over the background ({}), under the lowest node shown ({})", lowest - 1, floor, lowest);
    return lowest - 1;
  }
};
