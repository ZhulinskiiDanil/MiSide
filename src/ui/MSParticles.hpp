#pragma once

#include <Geode/Geode.hpp>

#include <random>
#include <vector>

// MiSide's motes: small magenta streaks flying the way they point, over the room, fading in and out. Each turns
// smoothly, so they fly in arcs and change direction; near ones take each other's heading, keep together and apart
// enough not to overlap, so they gather in small flocks; all drift a little along the theme's direction. Off one edge,
// in at the other. They move by the time that passed, so at the same speed at any frame rate; they take no input
class MSParticles : public cocos2d::CCNode
{
public:
  static MSParticles* create();

  void update(float dt) override;

protected:
  struct Mote
  {
    cocos2d::CCSprite* sprite = nullptr;
    cocos2d::CCPoint position;
    // Where it flies, in radians (0: right, counterclockwise), and how fast it's turning
    float heading = 0;
    float turn = 0;
    float speed = 0;
    float age = 0;
    float life = 0;
  };

  bool init() override;
  // A new life for it somewhere on the screen; `anyAge`: partway through, so they don't all start together
  void respawn(Mote& mote, bool anyAge);

  std::vector<Mote> m_motes;
  std::mt19937 m_random;
};
