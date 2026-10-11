#pragma once

#include "../ui/MSShine.hpp"
#include "../ui/ShinePass.hpp"

#include <Geode/Geode.hpp>

#include <string>
#include <vector>

// One of GD's achievements as the list shows it: read from AchievementManager when the screen opens
struct MSAchievement
{
  std::string id;
  std::string title;
  // The condition: GD's achieved or unachieved description, by its status
  std::string description;
  int percent = 0;
  bool earned = false;
  // From its icon ("ship_12"); none: GD's achievement picture instead
  bool hasReward = false;
  UnlockType type = UnlockType::Cube;
  int item = 0;
};

// A row of the list: a rounded see-through plate, the reward's icon in a white frame on the left, the title in the
// middle, a check when it's earned. Clicked, it opens: the condition, the status, the reward and the progress under
// the title. The list gives rows to whichever achievements are in view: bind() makes its content again for another
// one. Its origin is its top left corner
class MSAchievementRow : public cocos2d::CCNode
{
public:
  static MSAchievementRow* create(float width);

  void bind(MSAchievement const& achievement, int index, bool english);
  int index() const;
  // How much taller it is open: its details made (once per binding) and measured
  float openExtra();
  // Its height now (between closed and open) and how much of its details shows
  void setOpen(float extra, float shown);
  float height() const;
  // Lit by the mouse or the keys; shine: one pass through the title as it lights up
  void setLit(bool lit, bool shine);
  bool isLit() const;
  void step(float dt);

protected:
  bool init(float width);
  void buildDetails();
  void apply();

  MSAchievement m_achievement;
  int m_index = -1;
  bool m_english = false;
  float m_width = 0;
  float m_extra = 0;
  float m_lit = 0;
  bool m_litTarget = false;
  cocos2d::CCNode* m_plate = nullptr;
  cocos2d::CCNode* m_header = nullptr;
  cocos2d::CCNode* m_title = nullptr;
  cocos2d::CCNode* m_titleLabel = nullptr;
  float m_titleScale = 0;
  std::string m_titleText;
  cocos2d::CCNode* m_details = nullptr;
  float m_detailsHeight = 0;
  MSShine* m_shine = nullptr;
  MSShine* m_shineGlow = nullptr;
  bool m_shineTried = false;
  ShinePass m_shinePass;
};

// The achievements as MiSide's list: as many rows as fit in view, given in turn to whichever achievements are there,
// so hundreds of them cost no more than ten. It scrolls smoothly by the wheel, a drag or the scroll bar's thumb, and
// is cut at its top and bottom. The row under the mouse lights up with the menu's shine; a click (or Enter on the
// keys' row) opens it, and one opened closes the one open before
class MSAchievementList : public cocos2d::CCMenu
{
public:
  static MSAchievementList* create(std::vector<MSAchievement> achievements, bool english);

  // Up/Down and Enter, with their controller keys. True when used
  bool onKey(cocos2d::enumKeyCodes key);
  void setActive(bool active);

  void scrollWheel(float y, float x) override;
  void update(float dt) override;
  bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
  void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
  void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
  void ccTouchCancelled(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;

protected:
  bool init(std::vector<MSAchievement> achievements, bool english);
  float pitch() const;
  float viewHeight() const;
  // From the list's top, with the open rows' extra height
  float top(int index) const;
  float rowHeight(int index) const;
  float contentHeight() const;
  float maxScroll() const;
  // The rows given to the achievements in view, placed
  void layout();
  void toggle(int index);
  // Scrolled so all of it is in view
  void reveal(int index);
  int rowAt(cocos2d::CCPoint local);
  MSAchievementRow* rowFor(int index) const;
  void light(int index, bool shine);
  cocos2d::CCRect viewRect() const;
  cocos2d::CCRect trackRect() const;
  bool canUse();

  std::vector<MSAchievement> m_achievements;
  bool m_english = false;
  bool m_active = true;
  cocos2d::CCNode* m_clip = nullptr;
  std::vector<MSAchievementRow*> m_rows;
  cocos2d::CCNode* m_track = nullptr;
  cocos2d::CCNode* m_thumb = nullptr;
  float m_scroll = 0;
  float m_target = 0;
  // The open row and the one closing, each with its extra height now and its full one
  int m_open = -1;
  float m_openExtra = 0;
  float m_openFull = 0;
  int m_closing = -1;
  float m_closingExtra = 0;
  float m_closingFull = 0;
  // Kept where it is on screen while the rows around it open and close: the row clicked
  int m_anchor = -1;
  int m_lit = -1;
  int m_cursor = -1;
  bool m_touching = false;
  bool m_dragging = false;
  bool m_moved = false;
  cocos2d::CCPoint m_touchStart;
  float m_scrollStart = 0;
  cocos2d::CCPoint m_lastMouse;
};
