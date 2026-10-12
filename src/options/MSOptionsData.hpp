#pragma once

#include <Geode/Geode.hpp>

#include <string>
#include <vector>

// What GD's settings windows are made of, as they make it: every toggle their own addToggle adds (its text, its game
// variable or id, its info, its page) with the toggle button GD made for it, and the key bindings' pairs. Taken from
// hooks on those functions, so the toggles other mods add the same way are there too
namespace ms::options
{
  struct Toggle
  {
    // GD's own text: English, a line break where it has two lines
    std::string label;
    // The game variable ("0026"), or the id for the pause's options
    std::string key;
    std::string info;
    // GD's page it's on
    int page = 0;
    // GD's button for it: activating it does what a click on it does (its callback and its look)
    geode::Ref<CCMenuItemToggler> toggler;
  };

  struct KeyPair
  {
    // An action and its keys; a group's name has no keys
    std::string action;
    std::string keys;
  };

  std::vector<Toggle> const& toggles(MoreOptionsLayer* layer);
  std::vector<Toggle> const& toggles(MoreVideoOptionsLayer* layer);
  std::vector<Toggle> const& toggles(ParentalOptionsLayer* layer);
  std::vector<Toggle> const& toggles(GJOptionsLayer* layer);
  std::vector<KeyPair> const& keyPairs(KeybindingsLayer* layer);

  // The toggle buttons in the layer, in its tree's order
  std::vector<CCMenuItemToggler*> togglers(cocos2d::CCNode* layer);

  // A layer made to be driven, never shown: its actions stopped (they'd keep it alive) before it goes
  void dispose(cocos2d::CCNode* layer);

  // GD's text on one line: its line breaks as spaces
  std::string oneLine(std::string const& text);
} // namespace ms::options
