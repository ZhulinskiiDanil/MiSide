#include "MSParticles.hpp"

#include "../Theme.hpp"
#include "MSStyle.hpp"

#include <cmath>

using namespace geode::prelude;

namespace
{
  // At most this many, whatever the theme says
  constexpr int MOST = 120;
  // The share of a mote's life spent fading in, and out
  constexpr float FADE = 0.2f;
  constexpr float PI = 3.14159265f;
  constexpr float TAU = 6.2831853f;
  // How fast a mote's turning wanders (radians a second, per square root of a second), and how it settles back
  constexpr float TURN_JITTER = 2.2f;
  constexpr float TURN_SETTLE = 0.8f;
  // Flocking, against `particle-flock`: taking the neighbours' heading, flying to their middle, keeping apart
  constexpr float ALIGN = 1.6f;
  constexpr float COHERE = 0.9f;
  constexpr float SEPARATE = 2.4f;
  // Closer than this share of the flock radius: too close
  constexpr float PERSONAL_SPACE = 0.2f;
  // The common drift along the theme's direction
  constexpr float FLOW = 0.25f;
  // Off-screen this far before it comes in at the other side
  constexpr float EDGE = 12;

  // -pi..pi
  float wrap(float angle)
  {
    angle = std::fmod(angle + PI, TAU);
    return (angle < 0 ? angle + TAU : angle) - PI;
  }
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
  mote.position = CCPoint(winSize.width * unit(m_random), winSize.height * unit(m_random));
  // Mostly along the theme's direction, either way along it, some across
  auto flow = theme.particleAngle * PI / 180;
  mote.heading = wrap(flow + (unit(m_random) < 0.5f ? 0 : PI) + (unit(m_random) - 0.5f) * 1.6f);
  mote.turn = (unit(m_random) - 0.5f) * std::max(theme.particleTurn, 0.f);
  mote.speed = std::max(theme.particleSpeed, 0.f) * (0.6f + 0.8f * unit(m_random));
  auto size = theme.particleSizeMin + (theme.particleSizeMax - theme.particleSizeMin) * unit(m_random);
  // A streak `particle-stretch` times longer than wide, as big as a round mote of `size` (its area kept)
  auto own = mote.sprite->getContentSize();
  if (own.width > 0 && own.height > 0)
  {
    auto stretch = std::sqrt(std::max(theme.particleStretch, 1.f));
    mote.sprite->setScaleX(size * stretch / own.width);
    mote.sprite->setScaleY(size / stretch / own.height);
  }
}

void MSParticles::update(float dt)
{
  auto const& theme = Theme::get();
  auto winSize = CCDirector::get()->getWinSize();
  dt = std::min(dt, 0.1f);
  std::normal_distribution<float> normal(0.f, 1.f);
  auto maxTurn = std::max(theme.particleTurn, 0.f);
  auto flock = std::max(theme.particleFlock, 0.f);
  auto radius = std::max(theme.particleFlockRadius, 1.f);
  auto flow = theme.particleAngle * PI / 180;

  // Each mote's steering from its neighbours, all from where they are now
  std::vector<float> steer(m_motes.size(), 0.f);
  if (flock > 0)
  {
    for (size_t i = 0; i < m_motes.size(); i++)
    {
      auto const& me = m_motes[i];
      float sumX = 0, sumY = 0, headX = 0, headY = 0, awayX = 0, awayY = 0;
      int neighbours = 0;
      for (size_t j = 0; j < m_motes.size(); j++)
      {
        if (i == j)
          continue;
        auto dx = m_motes[j].position.x - me.position.x;
        auto dy = m_motes[j].position.y - me.position.y;
        auto distance = std::hypot(dx, dy);
        if (distance > radius || distance <= 0)
          continue;
        neighbours++;
        sumX += dx;
        sumY += dy;
        headX += std::cos(m_motes[j].heading);
        headY += std::sin(m_motes[j].heading);
        if (distance < radius * PERSONAL_SPACE)
        {
          awayX -= dx / distance;
          awayY -= dy / distance;
        }
      }
      if (neighbours == 0)
        continue;
      auto turnTo = [&](float x, float y, float weight) {
        if (x != 0 || y != 0)
          steer[i] += wrap(std::atan2(y, x) - me.heading) * weight;
      };
      turnTo(headX, headY, ALIGN);
      turnTo(sumX, sumY, COHERE);
      turnTo(awayX, awayY, SEPARATE);
      steer[i] *= flock;
    }
  }

  for (size_t i = 0; i < m_motes.size(); i++)
  {
    auto& mote = m_motes[i];
    mote.age += dt;
    if (mote.age >= mote.life)
      this->respawn(mote, false);
    // The turning wanders and settles: arcs one way, then the other
    mote.turn += normal(m_random) * TURN_JITTER * std::sqrt(dt) - mote.turn * TURN_SETTLE * dt;
    mote.turn = std::clamp(mote.turn, -maxTurn, maxTurn);
    // Along the common direction, either way, whichever is nearer
    auto along = wrap(flow - mote.heading);
    auto back = wrap(flow + PI - mote.heading);
    auto drift = std::abs(along) < std::abs(back) ? along : back;
    mote.heading = wrap(mote.heading + (mote.turn + steer[i] + drift * FLOW) * dt);
    mote.position.x += std::cos(mote.heading) * mote.speed * dt;
    mote.position.y += std::sin(mote.heading) * mote.speed * dt;
    // Off one edge, in at the other
    if (mote.position.x < -EDGE)
      mote.position.x += winSize.width + EDGE * 2;
    else if (mote.position.x > winSize.width + EDGE)
      mote.position.x -= winSize.width + EDGE * 2;
    if (mote.position.y < -EDGE)
      mote.position.y += winSize.height + EDGE * 2;
    else if (mote.position.y > winSize.height + EDGE)
      mote.position.y -= winSize.height + EDGE * 2;

    auto t = mote.age / mote.life;
    auto fade = std::min({1.f, t / FADE, (1.f - t) / FADE});
    mote.sprite->setOpacity(static_cast<GLubyte>(theme.particleOpacity * std::max(fade, 0.f)));
    mote.sprite->setPosition(mote.position);
    // Pointing where it flies (cocos turns clockwise)
    mote.sprite->setRotation(-mote.heading * 180 / PI);
  }
}
