#pragma once

#include <Geode/Geode.hpp>

#include <vector>

// The scene behind the menu: the theme's layers, back to front, following the mouse by their depth.
// A missing layer is skipped; a missing back layer becomes the theme's gradient
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
    float depth = 0;
  };

  bool init(float strength);

  std::vector<Layer> m_layers;
  float m_strength = 0;
  cocos2d::CCPoint m_offset;
};
