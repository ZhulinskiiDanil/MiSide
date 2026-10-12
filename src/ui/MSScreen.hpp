#pragma once

#include "MSMenuItem.hpp"

#include <Geode/Geode.hpp>

#include <functional>
#include <string>
#include <vector>

// A whole screen of the mod's over the menu, as the achievements' is: the menu's background, the title on the left,
// a row of buttons at the bottom (BACK first), Mita on the right. It's a popup (so the menu knows it's covered) and
// opaque: the menu hides under it once it has come in through the menu's darkening, and shows again as it goes. Esc,
// a controller's B and BACK close it; the keys a screen doesn't use go nowhere else
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

protected:
  // The background, Mita, the title and BACK; false when the popup itself can't be made
  bool initScreen(std::string const& title);
  // Another button in the bottom row, after the ones there
  MSMenuItem* addBottomButton(std::string const& text, float width, std::function<void()> action);
  // A key the screen uses (Esc and B are taken before); true when used
  virtual bool onKey(cocos2d::enumKeyCodes key);
  // Into the darkening, then gone, the menu showing again as it comes off; `after` runs once it's gone, else closed()
  void close(std::function<void()> after = nullptr);
  // Once it's gone after a close with nothing to run
  virtual void closed();
  bool isClosing() const;
  void hideMenu(bool hidden);

  cocos2d::CCLayerColor* m_curtain = nullptr;
  std::vector<MSMenuItem*> m_bottom;
  float m_bottomRight = 0;
  geode::Ref<cocos2d::CCNode> m_hidden;
  cocos2d::CCPoint m_lastMouse;
  bool m_closing = false;
};
