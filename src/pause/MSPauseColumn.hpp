#pragma once

#include "../ui/MSMenuItem.hpp"

#include <Geode/Geode.hpp>

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// The pause menu's list as MiSide has it: see-through rows one under the other, Quit apart under them. A row runs
// its action; a slider row holds a dark track with a pink fill and a knob with the value on it, dragged with the
// mouse or moved with Left/Right; a choice row shows its value on its right and goes to the next one on a click
// (Left/Right go either way); a field row shows a number that's typed in after a click or Enter. A group's name can
// stand in a row's place. The mouse lights the row under it, the arrow keys (and a controller) the next one; Enter,
// Space or A runs it. Longer than its view, it scrolls by whole rows, keeping the lit row in view, with a bar on its
// right. Hidden, it takes nothing
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

  struct Field
  {
    // Shown on the row's right, read every frame while it isn't being typed in
    std::function<std::string()> get;
    // The typed text, once it's done: Enter, a click on it, another row chosen
    std::function<void(std::string const&)> set;
    // Left/Right: the value a step down (-1) or up (1); none: they do nothing
    std::function<void(int)> step;
    // What can be typed
    std::string allowed = "0123456789";
    size_t maxLength = 8;
  };

  static MSPauseColumn* create();

  // Before any row: the rows' width (the theme's pause row by default); a slider keeps to the row's right end
  void setRowWidth(float width);
  // Before finish(): the rows past this height (from the first one's top) scroll. 0: no limit
  void setViewHeight(float height);
  // sound: in place of the click (empty: the click)
  void addRow(std::string const& text, std::function<void()> action, std::string const& sound = "");
  void addSlider(std::string const& text, Slider slider);
  void addChoice(std::string const& text, Choice choice);
  void addField(std::string const& text, Field field);
  // A group's name in the next row's place: not a row, nothing to choose
  void addHeader(std::string const& text);
  // What the row added last is about, given to the info callback while it's lit
  void setInfo(std::string info);
  // Called with the lit row's info as the light moves (empty: none lit, or it has none)
  void setOnInfo(std::function<void(std::string const&)> callback);
  // Apart, under the others; sound: in place of the click
  void setExit(std::string const& text, std::function<void()> action, std::string const& sound = "");
  // Places the rows from this top edge down, `left` their left edge
  void finish(float left, float firstTop);
  // Fades out (taking nothing from the first frame) or in; at once with animate off. Shown again, the row chosen
  // when it hid is chosen again (no new shine), until the mouse lights another
  void setShown(bool shown, bool animate = true);
  bool isShown() const;
  // True when the key was used
  bool onKey(cocos2d::enumKeyCodes key);
  // A field being typed in stops, its text dropped: true when one was (Esc goes no further then)
  bool cancelEdit();
  // For the mod's self-test: the rows' texts and what they show, and a row pressed as a click would
  std::vector<std::string> rowTexts() const;
  std::vector<std::string> rowValues();
  void pressRow(size_t index);
  // A choice or a slider moved a step either way, as Left/Right do; false when the row has nothing to move
  bool stepRow(size_t index, int direction);
  // A slider's exact value (0..1), and put back to one
  std::optional<float> sliderValue(size_t index);
  void setSliderValue(size_t index, float value);

  void update(float dt) override;
  // Over a slider's row, the wheel moves it as Left/Right do; elsewhere it scrolls a long column
  void scrollWheel(float y, float x) override;
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

  struct FieldRow
  {
    MSMenuItem* item = nullptr;
    Field field;
    cocos2d::CCNodeRGBA* control = nullptr;
    cocos2d::CCLabelBMFont* value = nullptr;
    std::string shown;
  };

  bool init() override;
  // reserveRight: room kept for a control on the row's right; the label is made smaller to end before it
  MSMenuItem* makeItem(std::string const& text, std::function<void()> action, std::string const& sound = "",
                       float reserveRight = 0);
  // A value label on the row's right, inside its padding, in a control node that fades with the row
  cocos2d::CCLabelBMFont* addValue(MSMenuItem* item, cocos2d::CCNodeRGBA*& control);
  float sliderLeft() const;
  void layoutSlider(SliderRow& row, float value);
  // The track's span in the column's space
  cocos2d::CCRect trackRect(SliderRow const& row) const;
  cocos2d::CCRect resetRect(SliderRow const& row) const;
  SliderRow* sliderFor(MSMenuItem* item);
  ChoiceRow* choiceFor(MSMenuItem* item);
  FieldRow* fieldFor(MSMenuItem* item);
  void showChoice(ChoiceRow& row);
  void showField(FieldRow& row);
  void dragTo(SliderRow& row, cocos2d::CCPoint local);
  void highlight(int index, bool shine);
  bool canUse();
  // Typing in the field at this index of m_fields; the one typed in before is done
  void beginEdit(size_t index);
  void commitEdit();
  bool onEditKey(cocos2d::enumKeyCodes key);
  // ! --- Scrolling --- !
  void placeRows();
  bool inView(cocos2d::CCNode* node) const;
  // The first slot shown, clamped; the lit row out of view goes dark
  void scrollTo(int offset);
  // The item at this index of m_items in view, with its group's name when it's right over it
  void reveal(int index);

  // The rows then Quit: the keys' order
  std::vector<MSMenuItem*> m_items;
  std::vector<MSMenuItem*> m_rows;
  // The rows and the groups' names, in their places' order
  std::vector<cocos2d::CCNode*> m_layout;
  std::vector<cocos2d::CCNode*> m_headers;
  MSMenuItem* m_exit = nullptr;
  std::vector<SliderRow> m_sliders;
  std::vector<ChoiceRow> m_choices;
  std::vector<FieldRow> m_fields;
  std::unordered_map<MSMenuItem*, std::string> m_infos;
  std::function<void(std::string const&)> m_onInfo;
  float m_rowWidth = 0;
  int m_shown = -1;
  // The row chosen when it hid (a confirmation, More), chosen again when it shows
  int m_restore = -1;
  bool m_isShown = true;
  cocos2d::CCPoint m_lastMouse;
  // A slider being dragged, or a reset button pressed
  SliderRow* m_dragging = nullptr;
  SliderRow* m_resetting = nullptr;
  // The field typed in (an index of m_fields, -1: none), its text, and whether the first key replaces it
  int m_editing = -1;
  std::string m_editText;
  bool m_editFresh = false;
  float m_blink = 0;
  // Where finish() put the rows, the view's height in slots and the first slot shown
  float m_left = 0;
  float m_firstTop = 0;
  float m_viewHeight = 0;
  int m_visibleSlots = 0;
  int m_offset = 0;
  cocos2d::CCNode* m_track = nullptr;
  cocos2d::CCNode* m_thumb = nullptr;
};
