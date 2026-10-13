#pragma once

#include "MSMenuItem.hpp"
#include "MSWave.hpp"

#include <Geode/Geode.hpp>

#include <functional>
#include <string>
#include <vector>

// A page of the menu, as MiSide's: its title where MAIN MENU is and as it looks, a column under it, a row of buttons
// at the bottom (BACK first). Over the main menu or another page it comes in place: the logo, the background and
// Mita stay, the page under it slides out to the right while this one slides in from the left (ms::wave), and back
// the same way as it closes. Elsewhere (over a level's page, the editor) it brings its own background, logo and
// Mita, coming in through a darkening. It's a popup either way (so what's under it knows it's covered) and every
// touch is its own. Esc, a controller's B and BACK close it; the keys a screen doesn't use go nowhere else
class MSScreen : public FLAlertLayer
{
public:
  ~MSScreen() override;

  void show() override;
  void registerWithTouchDispatcher() override;
  // Every touch is the screen's: nothing reaches the hidden menu
  bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
  void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
  void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
  void ccTouchCancelled(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
  void keyBackClicked() override;
  void keyDown(cocos2d::enumKeyCodes key, double timestamp) override;
  void update(float dt) override;

  static bool english();
  // The theme's points are on the 320 pt high reference screen, measured from its top
  static float fromTop();

  // What goes and comes as a page: its title, what's under it from the top down, its bottom row
  ms::wave::Steps pageSteps();

protected:
  // The background, Mita, the title and BACK; false when the popup itself can't be made. overlay: over a GD window
  // that stays in use under it (an editor of its own): no background, no Mita, no darkening, only a shade on the
  // left for the column, and a touch off its buttons goes on to the window
  bool initScreen(std::string const& title, bool overlay = false);
  // Another button in the bottom row, after the ones there
  MSMenuItem* addBottomButton(std::string const& text, float width, std::function<void()> action);
  // What the screen shows between its title and its bottom row, for its transitions (any order: they go from the
  // top down by where they are)
  virtual ms::wave::Steps contentSteps();
  // A node of its own moving with the page as a step of its own (one contentSteps() doesn't give)
  void addToPage(cocos2d::CCNode* node);
  // A node moving with the title, on its line (a count next to it)
  void addToTitle(cocos2d::CCNode* node);
  // A key the screen uses (Esc and B are taken before); true when used
  virtual bool onKey(cocos2d::enumKeyCodes key);
  // Esc, B or BACK, before the screen closes: true when the screen used it (a confirmation of its own closed)
  virtual bool onBack();
  // The bottom row's buttons can't be pressed (a confirmation of the screen's is open)
  void setBottomEnabled(bool enabled);
  // Out (the page under it coming back), or into the darkening; `after` runs once it's gone, else closed()
  void close(std::function<void()> after = nullptr);
  // As it starts closing (what it holds stops taking input)
  virtual void closing();
  // Once it's gone after a close with nothing to run
  virtual void closed();
  bool isClosing() const;
  void hideMenu(bool hidden);
  // The page it came in over, if any: a screen, or the main menu
  cocos2d::CCNode* findHost();
  ms::wave::Steps hostSteps();
  // Gone: off the touches and keys, out of the scene, then `after`
  void remove(std::function<void()> after);

  cocos2d::CCLayerColor* m_curtain = nullptr;
  std::vector<MSMenuItem*> m_bottom;
  float m_bottomRight = 0;
  geode::Ref<cocos2d::CCNode> m_hidden;
  cocos2d::CCPoint m_lastMouse;
  bool m_closing = false;
  bool m_overlay = false;
  // In place over the page it came in over (a screen, the main menu)
  bool m_inPlace = false;
  geode::WeakRef<cocos2d::CCNode> m_host;
  std::vector<cocos2d::CCNode*> m_titleNodes;
  std::vector<cocos2d::CCNode*> m_pageNodes;
  // An overlay's shade under its column
  cocos2d::CCNode* m_shade = nullptr;
  // GD may send Esc both as a key and as back on one frame: one back a frame
  unsigned m_backFrame = 0;
};
