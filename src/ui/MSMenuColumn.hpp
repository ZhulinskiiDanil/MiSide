#pragma once

#include "MSMenuItem.hpp"

#include <Geode/Geode.hpp>

#include <functional>
#include <string>
#include <vector>

// The column of items on the left. The mouse, the arrow keys and Enter move one highlight between them
class MSMenuColumn : public cocos2d::CCMenu
{
public:
  using Choose = std::function<void(MSMenuItem*)>;

  static MSMenuColumn* create(Choose choose);

  void addEntry(std::string const& text, cocos2d::CCMenuItem* target);
  // Lays the items out from the top and plays their intro
  void finish();
  // Puts the column's top edge at `top` on the theme's left edge, scaled down when it wouldn't end above `bottom`
  void fit(float top, float bottom);
  // What the column takes on the screen, in its parent's space
  cocos2d::CCRect getArea();
  // While an item's transition runs, nothing else is chosen
  void setBusy(bool busy);
  // From MenuLayer::keyDown. True when the key was used
  bool onKey(cocos2d::enumKeyCodes key);

  void update(float dt) override;

protected:
  bool init(Choose choose);
  void highlight(int index, bool sound);
  void choose(MSMenuItem* item);
  // Nothing (a popup, a scene transition) is over the menu
  bool isOnTop();

  Choose m_choose;
  std::vector<MSMenuItem*> m_items;
  int m_current = -1;
  // The items' bounds at scale 1
  float m_width = 0;
  float m_height = 0;
  cocos2d::CCPoint m_lastMouse;
  bool m_busy = false;
};
