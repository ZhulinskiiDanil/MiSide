#pragma once

#include <Geode/Geode.hpp>

#include <string>
#include <vector>

// The tray in the bottom right corner for the buttons other mods add, and the vanilla ones without an item in
// the column. The menus holding them stay alive with their layouts and their buttons: the tray only moves and
// scales each menu as a whole, and draws a panel and a slot under every button.
//
// Deferred: the first arrangement is on the first frame, after every mod's MenuLayer::init, and it's done again
// whenever a menu's buttons change (a mod adds one later and calls updateLayout())
class MSModTray : public cocos2d::CCNode
{
public:
  // The menus whose buttons go in the tray, by their node IDs
  static MSModTray* create(std::vector<std::string> sources);

  void update(float dt) override;

protected:
  struct Piece
  {
    cocos2d::CCNode* menu = nullptr;
    // The visible buttons' bounds, in the menu's space
    cocos2d::CCRect box;
    bool column = false;
  };

  bool init(std::vector<std::string> sources);
  std::vector<Piece> collect(std::vector<float>& signature);
  void arrange(std::vector<Piece> const& pieces);
  void place(cocos2d::CCNode* menu, float scale, cocos2d::CCPoint corner, cocos2d::CCPoint target);
  cocos2d::CCNode* slot(size_t index);

  std::vector<std::string> m_sources;
  std::vector<std::string> m_warned;
  std::vector<float> m_signature;
  cocos2d::CCNode* m_panel = nullptr;
  std::vector<cocos2d::CCNode*> m_slots;
};
