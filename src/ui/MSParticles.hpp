#pragma once

#include <Geode/Geode.hpp>

#include <random>
#include <vector>

// A few small pink motes drifting slowly up over the room, fading in and out. They move by the time that
// passed, so at the same speed at any frame rate; they take no input
class MSParticles : public cocos2d::CCNode
{
public:
  static MSParticles* create();

  void update(float dt) override;

protected:
  struct Mote
  {
    cocos2d::CCSprite* sprite = nullptr;
    cocos2d::CCPoint start;
    float age = 0;
    float life = 0;
    float sway = 0;
    float phase = 0;
    float rise = 0;
  };

  bool init() override;
  // A new life for it somewhere on the screen; `anyAge`: partway through, so they don't all start together
  void respawn(Mote& mote, bool anyAge);

  std::vector<Mote> m_motes;
  std::mt19937 m_random;
};
