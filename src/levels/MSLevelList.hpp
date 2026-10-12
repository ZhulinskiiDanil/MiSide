#pragma once

#include "../ui/MSMenuItem.hpp"

#include <Geode/Geode.hpp>

#include <functional>
#include <string>
#include <vector>

// The official levels as MiSide's list of saves: one row each, `levels-visible-rows` of them in view, the chosen one
// lit, a scroll bar on the right. Choosing a row (a click, Up/Down) calls back; a click also activates it. The mouse wheel, the
// keys and the scroll bar's thumb scroll it by whole rows, so a row is never cut (no clipping needed); rows slide and
// the ones leaving fade
class MSLevelList : public cocos2d::CCMenu
{
public:
  // reserveRight: room kept on each row's right for its accessory
  static MSLevelList* create(std::vector<std::string> const& names, std::function<void(int)> chosen,
                             float reserveRight = 0);
  // The row under the mouse, as soon as it comes on (-1: off the rows)
  void setOnHover(std::function<void(int)> hover);
  // A row clicked (after it's chosen)
  void setOnActivate(std::function<void(int)> activate);

  // Chooses it and scrolls it into view
  void choose(int index, bool sound = false);
  int chosen() const;
  // The first row in view: kept between visits
  int offset() const;
  void setOffset(int offset);
  // A small node on a row's right (its lock or progress), in the row's space; it fades with the row
  void setAccessory(int index, cocos2d::CCNode* node);
  // Each time the rows scroll
  void setOnScroll(std::function<void(int)> scrolled);
  // Fades out (taking nothing from the first frame) or in
  void setShown(bool shown);
  // Up/Down and their controller keys. True when used
  bool onKey(cocos2d::enumKeyCodes key);

  void scrollWheel(float y, float x) override;
  void update(float dt) override;
  bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
  void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
  void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
  void ccTouchCancelled(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;

protected:
  bool init(std::vector<std::string> const& names, std::function<void(int)> chosen, float reserveRight);
  void scrollTo(int offset, bool animate);
  void layout(bool animate);
  int visible() const;
  cocos2d::CCRect trackRect() const;
  bool canUse();

  std::function<void(int)> m_onChosen;
  std::function<void(int)> m_onHover;
  std::function<void(int)> m_onActivate;
  std::function<void(int)> m_onScroll;
  // A swipe: the touch's start and the first row in view then; swiping once it moved far enough
  bool m_tracking = false;
  bool m_swiping = false;
  float m_swipeFrom = 0;
  int m_swipeOffset = 0;
  std::vector<MSMenuItem*> m_rows;
  cocos2d::CCNode* m_track = nullptr;
  cocos2d::CCNode* m_thumb = nullptr;
  int m_chosen = -1;
  int m_offset = 0;
  int m_hover = -1;
  bool m_shown = true;
  bool m_dragging = false;
  cocos2d::CCPoint m_lastMouse;
};
