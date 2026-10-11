#pragma once

#include <Geode/Geode.hpp>

#include <string>

// The shine on an item's label: a wide soft wave of light, in two layers made once from one white render of the
// label (with room around it, and without its shadow):
// - the letters, over the label: near white in the wave's middle, back to their own color at its soft edges,
//   their outlines sharp;
// - the glow, under the label and its shadow: the letters' alpha blurred twice, tight and wide (a bloom), times a
//   wider wave, white, added to what's under it: white by the letters, fading far out. It's never multiplied by
//   the letters' own alpha again, so it reaches past them and the halos of neighbouring letters run together, but
//   only near the wave.
// The band is placed in the whole label's width (0: its left edge, 1: its right edge), never in the font atlas'
// per-letter coordinates. The shaders are the menu's own; each layer sets its pass right before it draws
class MSShine : public cocos2d::CCSprite
{
public:
  enum class Layer
  {
    Core,
    Glow,
  };

  struct Pair
  {
    MSShine* core = nullptr;
    MSShine* glow = nullptr;
  };

  // Both layers for this label, placed for a parent that has the label at its origin; empty when the shaders
  // don't compile here (logged once)
  static Pair create(std::string const& text, float scale, std::string const& font);

  // The band's middle in label widths, and how strong it is (0: not drawn)
  void setPass(float center, float strength);

  // How far the glow reaches past the letters, in label widths: the wave travels this much further on each side,
  // so the pass starts and ends with nothing showing
  float margin() const;

  void draw() override;

protected:
  // How much wider the glow's wave is than the letters'
  static float glowSpread();

  Layer m_layer = Layer::Core;
  // The label inside the texture: its offset and the texture's size, in label widths and heights
  cocos2d::CCPoint m_offset;
  cocos2d::CCSize m_span;
  float m_labelWidth = 0;
  float m_center = 0;
  float m_strength = 0;
};
