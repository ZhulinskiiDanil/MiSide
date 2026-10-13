#pragma once

#include "MSMenuItem.hpp"
#include "MSWave.hpp"

#include <Geode/Geode.hpp>

#include <functional>
#include <string>

// The quit confirmation as the column's own state: a title and two wide buttons, Yes and No, where the list was.
// It fades in with No chosen and takes nothing until it has; the mouse, the arrow keys and a controller choose
// between them. Yes calls back once only; No, Esc and a controller's B call `cancel`. Hidden, it takes nothing
class MSConfirm : public cocos2d::CCMenu
{
public:
  struct Texts
  {
    std::string title;
    std::string yes;
    std::string no;
  };

  // Where it goes, in points of the 569×320 screen
  struct Layout
  {
    float titleLeft = 0;
    float titleCapTop = 0;
    float titleScale = 0;
    float rowLeft = 0;
    float firstRowTop = 0;
    float rowPitch = 0;
    float rowWidth = 0;

    // In the main menu, under the logo
    static Layout menu();
  };

  // Null when its text can't be made (logged)
  static MSConfirm* create(Texts const& texts, std::function<void()> yes, std::function<void()> cancel,
                           Layout const& layout = Layout::menu(),
                           MSMenuItem::Style const& style = MSMenuItem::Style::confirm());

  // Fades in with No chosen (its shine passes, as for a key)
  void open();
  // Fades out; nothing is chosen from the first frame
  void close();
  bool isOpen() const;
  // Its question and buttons, from the top: what goes and comes as it opens and closes
  ms::wave::Steps pageSteps();
  // A key while it's open: every key is its, so none reaches what's hidden. Esc and B are `keyBack`
  void onKey(cocos2d::enumKeyCodes key);
  void keyBack();

  void update(float dt) override;

protected:
  bool init(Texts const& texts, std::function<void()> yes, std::function<void()> cancel, Layout const& layout,
            MSMenuItem::Style const& style);
  // It has faded in, it isn't fading out, nothing is over it, and Yes hasn't been chosen
  bool canChoose();
  void highlight(int index, bool shine);
  void choose(MSMenuItem* item);
  void setShown(float opacity, float time);

  std::function<void()> m_yes;
  std::function<void()> m_cancel;
  cocos2d::CCMenuItem* m_title = nullptr;
  cocos2d::CCNodeRGBA* m_titleText = nullptr;
  // Yes, then No
  std::vector<MSMenuItem*> m_items;
  int m_shown = -1;
  bool m_open = false;
  // Since it was opened: nothing is chosen before it has faded in
  float m_age = 0;
  bool m_done = false;
  cocos2d::CCPoint m_lastMouse;
};
