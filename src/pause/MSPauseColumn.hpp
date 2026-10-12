#pragma once

#include "../ui/MSMenuItem.hpp"

#include <Geode/Geode.hpp>

#include <functional>
#include <string>
#include <vector>

// The pause menu's list as MiSide has it: see-through rows one under the other, Quit apart under them. A row runs
// its action; a slider row holds a dark track with a pink fill and a knob with the value on it, dragged with the
// mouse or moved with Left/Right; a choice row shows its value on its right and goes to the next one on a click
// (Left/Right go either way). A group's name can stand in a row's place. The mouse lights the row under it, the arrow
// keys (and a controller) the next one; Enter, Space or A runs it. Hidden, it takes nothing
class MSPauseColumn : public cocos2d::CCMenu
{
public:
  struct Slider
  {
    // 0..1, read every frame: a change made elsewhere (GD's settings) shows
    std::function<float()> get;
    std::function<void(float)> set;
    // 0..1 a reset button sets; below 0: no button
    float reset = -1;
    // The value on the knob; none: 0..100
    std::function<std::string(float)> format;
  };

  struct Choice
  {
    // Shown on the row's right, read every frame
    std::function<std::string()> value;
    // To the next value (1) or the one before (-1)
    std::function<void(int)> change;
  };

  static MSPauseColumn* create();

  // sound: in place of the click (empty: the click)
  void addRow(std::string const& text, std::function<void()> action, std::string const& sound = "");
  void addSlider(std::string const& text, Slider slider);
  void addChoice(std::string const& text, Choice choice);
  // A group's name in the next row's place: not a row, nothing to choose
  void addHeader(std::string const& text);
  // Apart, under the others
  void setExit(std::string const& text, std::function<void()> action);
  // Places the rows from this top edge down, `left` their left edge
  void finish(float left, float firstTop);
  // Fades out (taking nothing from the first frame) or in; at once with animate off
  void setShown(bool shown, bool animate = true);
  bool isShown() const;
  // True when the key was used
  bool onKey(cocos2d::enumKeyCodes key);

  void update(float dt) override;
  bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
  void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
  void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
  void ccTouchCancelled(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;

protected:
  struct SliderRow
  {
    MSMenuItem* item = nullptr;
    Slider slider;
    // Its track, fill, knob and value, fading with the row
    cocos2d::CCNodeRGBA* control = nullptr;
    cocos2d::CCNode* fill = nullptr;
    cocos2d::CCNode* knob = nullptr;
    cocos2d::CCLabelBMFont* value = nullptr;
    cocos2d::CCNode* reset = nullptr;
    float shown = -1;
  };

  struct ChoiceRow
  {
    MSMenuItem* item = nullptr;
    Choice choice;
    cocos2d::CCNodeRGBA* control = nullptr;
    cocos2d::CCLabelBMFont* value = nullptr;
    std::string shown;
  };

  bool init() override;
  MSMenuItem* makeItem(std::string const& text, std::function<void()> action, std::string const& sound = "");
  void layoutSlider(SliderRow& row, float value);
  // The track's span in the column's space
  cocos2d::CCRect trackRect(SliderRow const& row) const;
  cocos2d::CCRect resetRect(SliderRow const& row) const;
  SliderRow* sliderFor(MSMenuItem* item);
  ChoiceRow* choiceFor(MSMenuItem* item);
  void showChoice(ChoiceRow& row);
  void dragTo(SliderRow& row, cocos2d::CCPoint local);
  void highlight(int index, bool shine);
  bool canUse();

  // The rows then Quit: the keys' order
  std::vector<MSMenuItem*> m_items;
  std::vector<MSMenuItem*> m_rows;
  // The rows and the groups' names, in their places' order
  std::vector<cocos2d::CCNode*> m_layout;
  std::vector<cocos2d::CCNode*> m_headers;
  MSMenuItem* m_exit = nullptr;
  std::vector<SliderRow> m_sliders;
  std::vector<ChoiceRow> m_choices;
  int m_shown = -1;
  bool m_isShown = true;
  cocos2d::CCPoint m_lastMouse;
  // A slider being dragged, or a reset button pressed
  SliderRow* m_dragging = nullptr;
  SliderRow* m_resetting = nullptr;
};
