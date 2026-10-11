#include "Theme.hpp"
#include "ui/MSBackground.hpp"
#include "ui/MSLayoutGuard.hpp"
#include "ui/MSMenuColumn.hpp"
#include "ui/MSModTray.hpp"
#include "ui/MSMorePopup.hpp"
#include "ui/MSProfileCard.hpp"
#include "ui/MSTransition.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

namespace
{
  // A node by its ID, from Geode's IDs for MenuLayer (loader/src/ids/MenuLayer.cpp). Missing: a warning,
  // and the step that needed it is skipped
  CCNode* find(CCNode* parent, std::string_view id, bool required = true)
  {
    auto node = parent ? parent->getChildByID(id) : nullptr;
    if (!node && required)
      log::warn("Node {} not found, skipped", id);
    return node;
  }

  // A vanilla button in its menu, or wherever another mod moved it
  CCMenuItem* findButton(CCNode* layer, std::string_view menu, std::string_view id)
  {
    auto node = find(find(layer, menu, false), id, false);
    if (!node)
      node = layer->getChildByIDRecursive(id);
    auto button = typeinfo_cast<CCMenuItem*>(node);
    if (!button)
      log::warn("Button {} not found, skipped", id);
    return button;
  }

  struct Entry
  {
    char const* ru;
    char const* en;
    // Where Geode's IDs put the vanilla button
    char const* menu;
    char const* button;
  };

  // Item -> the vanilla button it presses
  constexpr Entry ENTRIES[] = {
    {"Играть", "Play", "main-menu", "play-button"},
    {"Гардероб", "Wardrobe", "main-menu", "icon-kit-button"},
    {"Создать", "Create", "main-menu", "editor-button"},
    {"Настройки", "Settings", "bottom-menu", "settings-button"},
    {"Достижения", "Achievements", "bottom-menu", "achievements-button"},
    {"Статистика", "Statistics", "bottom-menu", "stats-button"},
    {"Выход", "Quit", "close-menu", "close-button"},
  };

  // The menus whose buttons go in the tray's row, in this order: the ones the other mods add theirs to, with the
  // vanilla buttons that do something and have no item (the daily chest, Newgrounds). profile-menu's own button
  // is the profile card; close-menu and top-right-menu stay at the top
  char const* const TRAY_SOURCES[] = {"bottom-menu", "right-side-menu", "profile-menu", "main-menu", "side-menu"};

  // The menus whose buttons are in the "…" popup instead: RobTop, the social links, More Games. Hidden as a whole,
  // their buttons stay in them
  char const* const POPUP_SOURCES[] = {"social-media-menu", "more-games-menu"};

  // Behind everything of the menu
  constexpr int BACKGROUND_Z = -100;
  // Under the vanilla menus (the tray's slots go under their buttons)
  constexpr int TRAY_Z = -1;
  // Above the vanilla menus, below the transition
  constexpr int COLUMN_Z = 10;
  // Over the whole menu
  constexpr int TRANSITION_Z = 1000;

  // Hidden, never removed: the node stays in the tree for the mods that look for it
  void hide(CCNode* node)
  {
    if (node)
      node->setVisible(false);
  }
} // namespace

class $modify(MSMenuLayer, MenuLayer)
{
  static void onModify(auto& self)
  {
    // After the other mods' init: the buttons they add are already there
    if (self.setHookPriorityPost("MenuLayer::init", Priority::VeryLate).isErr())
      log::warn("Can't set the MenuLayer::init hook's priority, buttons added after it are picked up a frame later");
  }

  struct Fields
  {
    MSMenuColumn* m_column = nullptr;
    MSTransition* m_transition = nullptr;
    MSProfileCard* m_card = nullptr;
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
    this->addTransition();
    this->addProfileCard();
    this->addTray();
    this->addGuard();
    return true;
  }

  void keyDown(enumKeyCodes key, double timestamp)
  {
    if (auto column = m_fields->m_column)
      column->onKey(key);
    // Always passed on: the mods hooking keyDown after this one get every key
    MenuLayer::keyDown(key, timestamp);
  }

  // ! --- Vanilla, hidden --- !

  void hideVanilla()
  {
    // The background with the icons running: hidden, and clicks no longer reach its icons
    if (auto background = find(this, "main-menu-bg"))
    {
      hide(background);
      if (auto layer = typeinfo_cast<CCLayer*>(background))
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
    auto column = MSMenuColumn::create([this](MSMenuItem* item) { this->runItem(item); });
    if (!column)
      return;
    auto english = this->isEnglish();
    if (Mod::get()->getSettingValue<bool>("show-title"))
      column->setTitle("Geometry Dash");
    std::vector<CCNode*> menus;
    for (auto const& entry : ENTRIES)
    {
      // No vanilla button, no item
      auto button = findButton(this, entry.menu, entry.button);
      if (!button)
        continue;
      column->addEntry(english ? entry.en : entry.ru, button);
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
  }

  void addTransition()
  {
    auto transition = MSTransition::create();
    if (!transition)
      return;
    transition->setID("transition"_spr);
    this->addChild(transition, TRANSITION_Z);
    m_fields->m_transition = transition;
  }

  // ! --- Running an item --- !

  // The darkening, then the vanilla button's activate(): its callback, and every mod's hook on it, run as usual
  void runItem(MSMenuItem* item)
  {
    Ref<CCMenuItem> target = item->getTarget();
    auto column = m_fields->m_column;
    auto transition = m_fields->m_transition;
    if (!target || !column)
      return;
    if (!transition)
    {
      target->activate();
      return;
    }
    column->setBusy(true);
    transition->play([target] { target->activate(); }, [column] { column->setBusy(false); });
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
    if (auto text = typeinfo_cast<CCLabelBMFont*>(label))
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
  std::vector<CCMenuItem*> popupButtons()
  {
    std::vector<CCMenuItem*> buttons;
    for (auto id : POPUP_SOURCES)
    {
      auto menu = find(this, id, false);
      if (!menu)
        continue;
      for (auto child : CCArrayExt<CCNode*>(menu->getChildren()))
      {
        if (auto button = typeinfo_cast<CCMenuItem*>(child); button && button->isVisible())
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
      more = [this] { this->openPopup(); };
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
    auto guard = MSLayoutGuard::create(column, m_fields->m_card, std::move(skipped));
    if (!guard)
      return;
    guard->setID("layout-guard"_spr);
    this->addChild(guard);
  }
};
