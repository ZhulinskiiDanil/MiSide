#include "MSParticles.hpp"

#include "../Theme.hpp"
#include "MSStyle.hpp"

#include <cmath>

using namespace geode::prelude;

namespace
{
  // At most this many, whatever the theme says
  constexpr int MOST = 64;
  // The share of a mote's life spent fading in, and out
  constexpr float FADE = 0.25f;
  // Side to side, in points, and how often
  constexpr float SWAY = 6;
  constexpr float SWAY_SPEED = 0.6f;
  constexpr float TAU = 6.2831853f;
} // namespace

MSParticles* MSParticles::create()
{
  auto particles = new MSParticles();
  if (particles->init())
  {
    particles->autorelease();
    return particles;
  }
  delete particles;
  return nullptr;
}

bool MSParticles::init()
{
  if (!CCNode::init())
    return false;
  auto const& theme = Theme::get();
  auto count = std::clamp(static_cast<int>(theme.particleCount), 0, MOST);
  if (count == 0 || !ms::resourceExists(theme.glowSprite))
    return true;
  m_random.seed(std::random_device{}());
  this->setContentSize(CCDirector::get()->getWinSize());
  for (int i = 0; i < count; i++)
  {
    auto sprite = CCSprite::create(theme.glowSprite.c_str());
    if (!sprite)
      break;
    sprite->setColor(theme.particleColor);
    // Light added to the room: a glint, never a dark spot
    sprite->setBlendFunc({GL_SRC_ALPHA, GL_ONE});
    this->addChild(sprite);
    Mote mote;
    mote.sprite = sprite;
    this->respawn(mote, true);
    m_motes.push_back(mote);
  }
  this->scheduleUpdate();
  return true;
}

void MSParticles::respawn(Mote& mote, bool anyAge)
{
  auto const& theme = Theme::get();
  auto winSize = CCDirector::get()->getWinSize();
  std::uniform_real_distribution<float> unit(0.f, 1.f);
  auto life = std::max(theme.particleLife, 0.5f);
  mote.life = life * (0.7f + 0.6f * unit(m_random));
  mote.age = anyAge ? mote.life * unit(m_random) : 0;
  mote.start = CCPoint(winSize.width * unit(m_random), winSize.height * unit(m_random));
  mote.sway = SWAY * (0.5f + unit(m_random));
  mote.phase = TAU * unit(m_random);
  mote.rise = theme.particleSpeed * (0.6f + 0.8f * unit(m_random));
  auto size = theme.particleSizeMin + (theme.particleSizeMax - theme.particleSizeMin) * unit(m_random);
  // A streak `particle-stretch` times longer than wide, as big as a round mote of `size` (its area kept), leaning
  // `particle-angle` degrees (cocos turns clockwise)
  auto own = mote.sprite->getContentSize();
  if (own.width > 0 && own.height > 0)
  {
    auto stretch = std::sqrt(std::max(theme.particleStretch, 1.f));
    mote.sprite->setScaleX(size * stretch / own.width);
    mote.sprite->setScaleY(size / stretch / own.height);
  }
  mote.sprite->setRotation(-theme.particleAngle);
}

void MSParticles::update(float dt)
{
  auto const& theme = Theme::get();
  for (auto& mote : m_motes)
  {
    mote.age += dt;
    if (mote.age >= mote.life)
      this->respawn(mote, false);
    auto t = mote.age / mote.life;
    // In and out softly
    auto fade = std::min({1.f, t / FADE, (1.f - t) / FADE});
    mote.sprite->setOpacity(static_cast<GLubyte>(theme.particleOpacity * std::max(fade, 0.f)));
    mote.sprite->setPosition({mote.start.x + mote.sway * std::sin(mote.phase + mote.age * SWAY_SPEED * TAU / 2),
                              mote.start.y + mote.rise * mote.age});
  }
}
