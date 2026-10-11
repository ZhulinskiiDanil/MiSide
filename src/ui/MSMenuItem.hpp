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

  // How it looks at rest, lit and pressed
  struct Style
  {
    float height = 0;
    std::string font;
    cocos2d::ccColor3B plate;
    GLubyte plateOpacity = 0;
    cocos2d::ccColor3B plateLit;
    GLubyte plateLitOpacity = 0;
    cocos2d::ccColor3B platePressed;
    GLubyte platePressedOpacity = 0;
    cocos2d::ccColor3B text;
    cocos2d::ccColor3B textLit;
    cocos2d::ccColor3B textPressed;
    float textScale = 0;
    float textOffset = 0;
    float textPadding = 0;
    bool textShadow = true;
    // The soft glow around the lit row, and the shine through its letters
    bool glow = true;
    bool shine = true;

    // The menu's rows
    static Style row();
    // The quit confirmation's buttons
    static Style confirm();
    // The pause menu's rows
    static Style pause();
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
  // Fades it as a whole (the quit confirmation coming and going): its plate, text, accent and glow each end at their
  // own style's opacity times this, never at full
  void fadeTo(float time, GLubyte opacity);
  // Back to rest at once: not lit or pressed, its shine stopped, its look the style's (a screen of the menu changed)
  void resetLook();

  // Exit: it leans away from the mouse around this place (its middle, in the column's space)
  void setDodgeHome(cocos2d::CCPoint home);
  // Every frame, from the column: the mouse in the column's space, or null when there's no mouse to lean from
  // (the keyboard is in use, a popup is over the menu, a transition runs)
  void stepDodge(float dt, cocos2d::CCPoint const* mouse);

  void selected() override;
  void unselected() override;
  void activate() override;

protected:
  // What it looks like in one state, before its visibility: everything the style sets
  struct Look
  {
    cocos2d::ccColor3B plate;
    float plateOpacity = 0;
    cocos2d::ccColor3B text;
    float textOpacity = 0;
    float accent = 0;
    float glow = 0;
    float textX = 0;
    float scale = 1;
  };

  bool init(std::string const& text, cocos2d::CCMenuItem* target, Callback callback, float width, Style const& style);
  void addShine(std::string const& text);
  // The look for its state now (rest, lit, pressed, unavailable)
  Look target() const;
  // Goes to the look for its state, over `hover-duration` or at once
  void refresh(bool animate);
  // The one place the look and the visibility reach the nodes. Each opacity is set on its own node: nothing relies on
  // cascading opacity, which a 9-slice plate takes as its own (fading a parent to full made the plate opaque)
  void apply();
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
  Look m_look;
  Look m_from;
  Look m_to;
  float m_lookTime = 0;
  float m_lookDuration = 0;
  // 0..1: how much of it shows (the intro, the confirmation's fades), and where it's going, after a delay
  float m_visibility = 1;
  float m_visibilityFrom = 1;
  float m_visibilityTo = 1;
  float m_visibilityTime = 0;
  float m_visibilityDuration = 0;
  float m_visibilityDelay = 0;
  bool m_highlighted = false;
  bool m_pressed = false;
  bool m_unavailable = false;
  bool m_dodging = false;
  cocos2d::CCPoint m_dodgeHome;
  DodgeSpring m_dodge;
};
