#pragma once

#include <Geode/Geode.hpp>

#include <string>

// The shine on an item's label, in two layers made once from one white render of the label (with room around it):
// - the core, over the label: the letters turn white inside the moving band, their edges stay sharp;
// - the glow, under the label: the letters' alpha blurred, times the band, in the glow color, added to what's
//   under it. It's never multiplied by the letters' own alpha again, so it reaches past them, but only near the
//   band.
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

  // The room around the label for the glow, in label widths: the band travels this much further on each side, so
  // the pass starts and ends with no glow showing
  float margin() const { return m_offset.x; }

  void draw() override;

protected:
  Layer m_layer = Layer::Core;
  // The label inside the texture: its offset and the texture's size, in label widths and heights
  cocos2d::CCPoint m_offset;
  cocos2d::CCSize m_span;
  float m_center = 0;
  float m_strength = 0;
};
