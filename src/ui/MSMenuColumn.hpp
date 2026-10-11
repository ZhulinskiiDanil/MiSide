#pragma once

#include "MSMenuItem.hpp"

#include <Geode/Geode.hpp>

#include <functional>
#include <string>
#include <vector>

// The menu on the left: the logo, the subtitle, the rows from `first-row-y` down, and Quit apart near the bottom.
// One highlight for all of them: the row under the mouse, or else the one the arrow keys chose. Enter chooses it
class MSMenuColumn : public cocos2d::CCMenu
{
public:
  using Choose = std::function<void(MSMenuItem*)>;

  static MSMenuColumn* create(Choose choose);

  // The next row, `row-pitch` under the previous one
  void addEntry(std::string const& text, cocos2d::CCMenuItem* target);
  // Quit: shorter, on its own at `exit-y`, the last for the arrow keys
  void setExit(std::string const& text, cocos2d::CCMenuItem* target);
  void setLogo(cocos2d::CCNode* logo);
  void setSubtitle(std::string const& text);
  // Places everything and plays the intro
  void finish();
  // What the menu takes on the screen, in its parent's space
  cocos2d::CCRect getArea();
  // While an item's transition runs, nothing else is chosen
  void setBusy(bool busy);
  // From MenuLayer::keyDown. True when the key was used
  bool onKey(cocos2d::enumKeyCodes key);

  void update(float dt) override;

protected:
  bool init(Choose choose);
  MSMenuItem* makeItem(std::string const& text, cocos2d::CCMenuItem* target, float width);
  // A node that isn't an item: wrapped in a disabled one, as the menu holds items
  cocos2d::CCMenuItem* wrap(cocos2d::CCNode* node);
  // Shows the hovered row, or the keyboard's
  void showHighlight();
  void choose(MSMenuItem* item);
  // Nothing (a popup, a scene transition) is over the menu
  bool isOnTop();

  Choose m_choose;
  // The rows, then Quit: the arrow keys' order
  std::vector<MSMenuItem*> m_items;
  std::vector<MSMenuItem*> m_rows;
  MSMenuItem* m_exit = nullptr;
  cocos2d::CCMenuItem* m_logo = nullptr;
  cocos2d::CCMenuItem* m_subtitle = nullptr;
  cocos2d::CCNode* m_subtitleText = nullptr;
  int m_hover = -1;
  int m_keyboard = -1;
  int m_shown = -1;
  cocos2d::CCPoint m_lastMouse;
  bool m_busy = false;
};
