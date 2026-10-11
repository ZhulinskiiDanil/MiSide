#pragma once

#include <Geode/Geode.hpp>

#include <functional>
#include <string>

// A text item of the column. It doesn't run anything itself: its vanilla button does, through activate(), so
// the mods hooking that button's callback keep working
class MSMenuItem : public cocos2d::CCMenuItem
{
public:
  using Callback = std::function<void(MSMenuItem*)>;

  static MSMenuItem* create(std::string const& text, cocos2d::CCMenuItem* target, Callback callback);

  cocos2d::CCMenuItem* getTarget() const;
  void setHighlighted(bool highlighted);
  bool isHighlighted() const;
  // Slides in from the left when the menu opens
  void playIntro(float delay);

  void selected() override;
  void unselected() override;
  void activate() override;

protected:
  bool init(std::string const& text, cocos2d::CCMenuItem* target, Callback callback);
  void refresh(bool animate);

  geode::Ref<cocos2d::CCMenuItem> m_target;
  Callback m_callback;
  // The label and its accent bar move together on hover
  cocos2d::CCNode* m_content = nullptr;
  cocos2d::CCNode* m_label = nullptr;
  cocos2d::CCNode* m_accent = nullptr;
  float m_contentX = 0;
  bool m_highlighted = false;
  bool m_pressed = false;
};
