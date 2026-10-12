#pragma once

#include "MSLogo.hpp"
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
  void setLogo(MSLogo* logo);
  void setSubtitle(std::string const& text);
  // Places everything and plays the intro
  void finish();
  // What the menu takes on the screen, in its parent's space
  cocos2d::CCRect getArea();
  // While an item's transition runs, nothing else is chosen
  void setBusy(bool busy);
  void setOnExitHover(std::function<void(bool)> callback);
  bool isBusy() const { return m_busy; }
  // From MenuLayer::keyDown. True when the key was used
  bool onKey(cocos2d::enumKeyCodes key);
  // Quit
  MSMenuItem* getExit() const;
  // The quit confirmation takes the list's place: the rows, Quit and the subtitle fade out and take nothing from
  // the first frame (Quit stops leaning away), the logo stays. Back: they fade in, and if the keys chose Quit,
  // Quit is chosen again
  void setConfirming(bool confirming);
  bool isConfirming() const;
  // Nothing (a popup, a scene transition) is over the menu
  bool isOnTop();

  void update(float dt) override;

protected:
  bool init(Choose choose);
  MSMenuItem* makeItem(std::string const& text, cocos2d::CCMenuItem* target, float width);
  // A node that isn't an item: wrapped in a disabled one, as the menu holds items
  cocos2d::CCMenuItem* wrap(cocos2d::CCNode* node);
  // Shows the hovered row, or the keyboard's. shine: it lit up because of the mouse coming or a key
  void showHighlight(bool shine);
  // The rows' and Quit's rest places (Quit with room to lean away), in the column's space
  void measureArea();
  void choose(MSMenuItem* item);

  void notifyExitHover(bool hovered);
  std::function<void(bool)> m_onExitHover;
  bool m_exitHovered = false;
  Choose m_choose;
  // The rows, then Quit: the arrow keys' order
  std::vector<MSMenuItem*> m_items;
  std::vector<MSMenuItem*> m_rows;
  MSMenuItem* m_exit = nullptr;
  cocos2d::CCMenuItem* m_logo = nullptr;
  MSLogo* m_logoArt = nullptr;
  cocos2d::CCMenuItem* m_subtitle = nullptr;
  cocos2d::CCNode* m_subtitleText = nullptr;
  int m_hover = -1;
  int m_keyboard = -1;
  int m_shown = -1;
  cocos2d::CCPoint m_lastMouse;
  // The mouse moved since the last key: Quit leans away from it only then
  bool m_mouseActive = false;
  cocos2d::CCRect m_area;
  bool m_busy = false;
  bool m_confirming = false;
  // The keys had chosen Quit when the confirmation opened
  bool m_keysOnExit = false;
};
