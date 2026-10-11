#include "Theme.hpp"

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

  bool init()
  {
    if (!MenuLayer::init())
      return false;
    // The vanilla menu: nothing else happens here
    if (Mod::get()->getSettingValue<bool>("vanilla-menu"))
      return true;

    Theme::reload();
    this->hideVanilla();
    return true;
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
};
