#pragma once

#include "../pause/MSPauseColumn.hpp"
#include "../ui/MSScreen.hpp"

#include <Geode/Geode.hpp>

#include <string>
#include <vector>

// A screen of the mod's for GD's settings and the windows they open: paragraphs of text under the title, a column of
// rows under them (longer ones scrolling), and under it what the lit row does. The GD windows it works are made for
// it and never shown; they go with it. While one is open, GD's plain popups show as the mod's screens too
class MSOptionsScreen : public MSScreen
{
public:
  ~MSOptionsScreen() override;
  void onEnter() override;
  void onExit() override;
  // For the mod's self-test
  MSPauseColumn* column() const { return m_column; }
  bool closing() const { return this->isClosing(); }

protected:
  bool initOptions(std::string const& title, bool overlay = false);
  // A paragraph over the column, wrapped to the rows' width; the column starts under it
  void addText(std::string const& text);
  void finishOptions();
  // Everything addText and the rows put up, gone, for a column made anew (the window it shows changed)
  void resetOptions();
  // Wrapped to the rows' width under the column, smaller until it fits over the bottom row
  void showInfo(std::string const& text);

  // GD's window, kept for the rows to work
  template <class T>
  T* drive(T* layer)
  {
    if (layer)
      m_driven.emplace_back(layer);
    return layer;
  }

  // What a driven window does on a click: the popups it puts in itself (it isn't shown, so they wouldn't be) go over
  // this screen, the way they came in
  void press(cocos2d::CCNode* window, std::function<void()> const& action);

  bool onKey(cocos2d::enumKeyCodes key) override;
  // Esc while typing a number stops the typing, not the screen
  bool onBack() override;

  MSPauseColumn* m_column = nullptr;
  cocos2d::CCNode* m_info = nullptr;
  std::string m_infoText;
  // The column's top, in the 320 pt screen's points from its bottom
  float m_top = 0;
  std::vector<cocos2d::CCNode*> m_texts;
  std::vector<geode::Ref<cocos2d::CCNode>> m_driven;
};

namespace ms::options
{
  // A text wrapped to the width in the mod's font, its top left corner at its position; null when it can't be made
  cocos2d::CCNode* wrapped(std::string const& text, float scale, cocos2d::ccColor3B color, float width);
  // While a settings screen (or the pause's settings) is open, GD's plain popups show as the mod's screens
  bool restyling();
  void holdRestyling(bool hold);
  // The self-test is running: the screens log what they can't name
  bool selfTesting();
  void setSelfTesting(bool on);
} // namespace ms::options
