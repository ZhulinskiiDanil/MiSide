#pragma once

#include "DodgeSpring.hpp"
#include "MSShine.hpp"
#include "ShinePass.hpp"

#include <Geode/Geode.hpp>

#include <functional>
#include <string>

// An item of the menu: a plate the size of the row with the text on it, all of it clickable. It doesn't run
// anything itself: its vanilla button does, through activate(), so the mods hooking that button keep working. One
// without a vanilla button (the quit confirmation's) only calls back
class MSMenuItem : public cocos2d::CCMenuItem
{
public:
  using Callback = std::function<void(MSMenuItem*)>;

  // How it looks at rest; lit and pressed it's the same for all
  struct Style
  {
    float height = 0;
    std::string font;
    cocos2d::ccColor3B plate;
    GLubyte plateOpacity = 0;
    cocos2d::ccColor3B text;
    float textScale = 0;
    float textOffset = 0;
    float textPadding = 0;
    bool textShadow = true;

    // The menu's rows
    static Style row();
    // The quit confirmation's buttons
    static Style confirm();
  };

  // width: the row's width
  static MSMenuItem* create(std::string const& text, cocos2d::CCMenuItem* target, Callback callback, float width,
                            Style const& style = Style::row());

  cocos2d::CCMenuItem* getTarget() const;
  // Hover and the keyboard's choice look the same. shine: one light through the letters as it lights up
  void setHighlighted(bool highlighted, bool shine = true);
  bool isHighlighted() const;
  // Its vanilla button can't be pressed now
  bool isUnavailable() const;
  // Fades in when the menu opens
  void playIntro(float delay);
  // Its plate and text fade to this opacity (the quit confirmation coming and going)
  void fadeTo(float time, GLubyte opacity);

  // Exit: it leans away from the mouse around this place (its middle, in the column's space)
  void setDodgeHome(cocos2d::CCPoint home);
  // Every frame, from the column: the mouse in the column's space, or null when there's no mouse to lean from
  // (the keyboard is in use, a popup is over the menu, a transition runs)
  void stepDodge(float dt, cocos2d::CCPoint const* mouse);

  void selected() override;
  void unselected() override;
  void activate() override;

protected:
  bool init(std::string const& text, cocos2d::CCMenuItem* target, Callback callback, float width, Style const& style);
  void addShine(std::string const& text);
  void refresh(bool animate);
  void update(float dt) override;

  geode::Ref<cocos2d::CCMenuItem> m_target;
  Callback m_callback;
  Style m_style;
  // The plate, the text and the accent bar; its opacity fades them together
  cocos2d::CCNode* m_content = nullptr;
  cocos2d::CCNode* m_plate = nullptr;
  cocos2d::CCNode* m_glow = nullptr;
  cocos2d::CCNode* m_text = nullptr;
  cocos2d::CCNode* m_label = nullptr;
  cocos2d::CCNode* m_accent = nullptr;
  // The shine: its core over the label, its glow under it (drawn only during a pass), and this item's own pass
  MSShine* m_shine = nullptr;
  MSShine* m_shineGlow = nullptr;
  ShinePass m_shinePass;
  float m_textX = 0;
  bool m_highlighted = false;
  bool m_pressed = false;
  bool m_unavailable = false;
  bool m_dodging = false;
  cocos2d::CCPoint m_dodgeHome;
  DodgeSpring m_dodge;
};
