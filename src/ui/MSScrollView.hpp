#pragma once

#include <Geode/Geode.hpp>

#include <functional>

// A column of content that scrolls smoothly by the wheel, a drag or the scroll bar's thumb, cut at the view's top and
// bottom (sideways there's room for glows). The content node's children are placed from its top: x from the view's
// left, y at or below 0. A press that doesn't move is a tap, given in the content's own coordinates
class MSScrollView : public cocos2d::CCMenu
{
public:
  // view: on the screen; the bar at barLeft, barWidth wide, as tall as the view
  static MSScrollView* create(cocos2d::CCRect view, float barLeft, float barWidth, cocos2d::ccColor3B track,
                              cocos2d::ccColor3B thumb);

  cocos2d::CCNode* content() const;
  void setContentHeight(float height);
  // How far the wheel or an arrow key moves it
  void setStep(float step);
  void setOnTap(std::function<void(cocos2d::CCPoint)> tap);
  // From the content's top, down
  float scroll() const;
  // Scrolled (smoothly) so this band of the content, from its top down, is in view
  void reveal(float top, float bottom);
  void scrollBy(float amount);
  // Taking no input (the screen is going)
  void setActive(bool active);

  void scrollWheel(float y, float x) override;
  void update(float dt) override;
  bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
  void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
  void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
  void ccTouchCancelled(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;

protected:
  bool init(cocos2d::CCRect view, float barLeft, float barWidth, cocos2d::ccColor3B track, cocos2d::ccColor3B thumb);
  float maxScroll() const;
  void place();
  bool canUse();
  cocos2d::CCRect trackRect() const;

  cocos2d::CCRect m_view;
  cocos2d::CCNode* m_clip = nullptr;
  cocos2d::CCNode* m_content = nullptr;
  cocos2d::CCNode* m_track = nullptr;
  cocos2d::CCNode* m_thumb = nullptr;
  float m_barWidth = 0;
  float m_height = 0;
  float m_step = 20;
  float m_scroll = 0;
  float m_target = 0;
  bool m_active = true;
  bool m_dragging = false;
  bool m_touching = false;
  bool m_moved = false;
  cocos2d::CCPoint m_touchStart;
  float m_scrollStart = 0;
  std::function<void(cocos2d::CCPoint)> m_onTap;
};
