#pragma once

#include <Geode/Geode.hpp>

#include <string>

// The shine over an item's label: a white copy of the label, rendered once into a texture, drawn over the label
// with its own shader. The band is placed in the whole label's width (the copy's texture spans exactly the
// label), never in the font atlas' per-letter coordinates; it shows only through the letters' alpha, white in the
// middle, the tint at its edges. Its shader is shared by the items' shines and nothing else; each shine sets its
// own pass right before it draws
class MSShine : public cocos2d::CCSprite
{
public:
  // Null when the shine shader doesn't compile here (logged once)
  static MSShine* create(std::string const& text, float scale, std::string const& font);

  // The band's middle in label widths (0: the left edge, 1: the right edge), and how strong it is (0: not drawn)
  void setPass(float center, float strength);

  void draw() override;

protected:
  float m_center = 0;
  float m_strength = 0;
};
