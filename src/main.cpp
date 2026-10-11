#include "Theme.hpp"
#include "ui/MSBackground.hpp"
#include "ui/MSMenuColumn.hpp"
#include "ui/MSModTray.hpp"
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

  // The menus whose buttons go in the tray: the other mods add theirs here. The ones in the corners
  // (profile-menu, close-menu, top-right-menu, social-media-menu, more-games-menu) stay where they are
  char const* const TRAY_SOURCES[] = {"bottom-menu", "right-side-menu", "main-menu", "side-menu"};

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
    this->addTray();
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

    // An invisible button can't be clicked or laid out, but activate() still runs it
    for (auto id : {"play-button", "icon-kit-button", "editor-button"})
      hide(findButton(this, "main-menu", id));
  }

  // ! --- The background --- !

  void addBackground()
  {
    auto background = MSBackground::create(static_cast<float>(Mod::get()->getSettingValue<double>("parallax-strength")));
    background->setID("background"_spr);
    this->addChild(background, BACKGROUND_Z);
  }

  // ! --- The column --- !

  void addColumn()
  {
    auto column = MSMenuColumn::create([this](MSMenuItem* item) { this->runItem(item); });
    auto english = Mod::get()->getSettingValue<std::string>("menu-language") == "EN";
    std::vector<CCNode*> menus;
    for (auto const& entry : ENTRIES)
    {
      // No vanilla button, no item
      auto button = findButton(this, entry.menu, entry.button);
      if (!button)
        continue;
      column->addEntry(english ? entry.en : entry.ru, button);
      // The column has it now
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

    auto transition = MSTransition::create();
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

  // ! --- The tray --- !

  void addTray()
  {
    auto tray = MSModTray::create({std::begin(TRAY_SOURCES), std::end(TRAY_SOURCES)});
    tray->setID("mod-tray"_spr);
    this->addChild(tray, TRAY_Z);
  }
};
