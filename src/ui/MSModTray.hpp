#pragma once

#include <Geode/Geode.hpp>

#include <functional>
#include <string>
#include <vector>

// The tray in the bottom right corner: one row of same-size slots for the buttons other mods add and the vanilla
// ones without an item in the column, then a "…" button for the rest. Every visible button of the source menus is
// scaled to fit a slot and placed in it. The buttons stay in their menus with their callbacks, the menus stay
// where they are with their layouts: a mod can keep adding buttons and calling updateLayout().
//
// Deferred: the first arrangement is on the first frame, after every mod's MenuLayer::init, and it's done again
// whenever a button is added, hidden, or moved by a layout
class MSModTray : public cocos2d::CCNode
{
public:
  // sources: the menus whose buttons go in the row, by their node IDs, in this order.
  // more: what the "…" button at the end of the row does; none, no button
  static MSModTray* create(std::vector<std::string> sources, std::function<void()> more);

  void update(float dt) override;

protected:
  bool init(std::vector<std::string> sources, std::function<void()> more);
  // The visible buttons of the source menus, in order, and the "…" button. The signature changes when any moves
  std::vector<cocos2d::CCNode*> collect(std::vector<float>& signature);
  void arrange(std::vector<cocos2d::CCNode*> const& buttons);
  void place(cocos2d::CCNode* button, float size, cocos2d::CCPoint center);
  cocos2d::CCNode* slot(size_t index);

  std::vector<std::string> m_sources;
  std::vector<std::string> m_warned;
  std::vector<float> m_signature;
  cocos2d::CCNode* m_panel = nullptr;
  std::vector<cocos2d::CCNode*> m_slots;
  // Holds the "…" button, over the slots
  cocos2d::CCMenu* m_menu = nullptr;
  cocos2d::CCNode* m_more = nullptr;
};
