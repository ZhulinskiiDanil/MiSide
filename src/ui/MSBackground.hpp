#pragma once

#include <Geode/Geode.hpp>

#include <vector>

// The scene behind the menu, back to front: the theme's layers (each tinted, following the mouse by its depth),
// a darkening over all of them, a soft glow on the right, the character if the theme has one, and a shade on the
// left under the menu. A missing layer is skipped; a missing back layer becomes the theme's gradient
class MSBackground : public cocos2d::CCNode
{
public:
  // strength: the parallax-strength setting, 0 keeps the layers still
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
  void addLayers();
  void addOverlays();
  void addCharacter();
  // The faint repeating pattern over the room
  void addPattern();

  std::vector<Layer> m_layers;
  float m_strength = 0;
  cocos2d::CCPoint m_offset;
};
