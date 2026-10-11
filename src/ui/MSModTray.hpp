#pragma once

#include <Geode/Geode.hpp>

#include <string>
#include <vector>

// The tray in the bottom right corner for the buttons other mods add, and the vanilla ones without an item in
// the column. It's a grid of same-size slots: every visible button of the source menus is scaled to fit a slot
// and placed in it. The buttons stay in their menus with their callbacks, the menus stay where they are with
// their layouts: a mod can keep adding buttons and calling updateLayout().
//
// Deferred: the first arrangement is on the first frame, after every mod's MenuLayer::init, and it's done again
// whenever a button is added, hidden, or moved by a layout
class MSModTray : public cocos2d::CCNode
{
public:
  // The menus whose buttons go in the tray, by their node IDs, in the order their buttons are placed
  static MSModTray* create(std::vector<std::string> sources);

  void update(float dt) override;

protected:
  bool init(std::vector<std::string> sources);
  // The visible buttons of the source menus, in order. The signature changes when any of them moves
  std::vector<cocos2d::CCNode*> collect(std::vector<float>& signature);
  void arrange(std::vector<cocos2d::CCNode*> const& buttons);
  void place(cocos2d::CCNode* button, float size, cocos2d::CCPoint center);
  cocos2d::CCNode* slot(size_t index);

  std::vector<std::string> m_sources;
  std::vector<std::string> m_warned;
  std::vector<float> m_signature;
  cocos2d::CCNode* m_panel = nullptr;
  std::vector<cocos2d::CCNode*> m_slots;
};
