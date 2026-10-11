#pragma once

#include <Geode/Geode.hpp>

#include <vector>

// The scene behind the menu and its screens, back to front, as MiSide's: a dark gradient across (nearly black on
// the left, burgundy to the right), a faint pattern of thin outlined hearts, flowers and stars, a soft purple glow
// behind Mita, the pink motes, a shade on the left under the menu and dark edges. A theme may put pictures behind
// it all instead of the gradient (`layers`, each tinted, following the mouse by its depth, darkened together) and a
// still character. The pattern and the layers follow the mouse
class MSBackground : public cocos2d::CCNode
{
public:
  // strength: the parallax-strength setting, 0 keeps everything still
  static MSBackground* create(float strength);

  void update(float dt) override;

protected:
  struct Layer
  {
    cocos2d::CCNode* node = nullptr;
    cocos2d::CCPoint home;
    float depth = 0;
  };

  bool init(float strength);
  // The theme's pictures, darkened; none: the backdrop gradient
  void addLayers();
  void addBackdrop();
  void addOverlays();
  void addCharacter();
  // The faint repeating pattern
  void addPattern();
  // How far the layer deepest in moves, either way: what's laid under the screen's edges
  float margin(float depth) const;

  std::vector<Layer> m_layers;
  float m_strength = 0;
  cocos2d::CCPoint m_offset;
};
