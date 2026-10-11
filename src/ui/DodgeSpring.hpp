#pragma once

#include <algorithm>
#include <cmath>

// The Exit button leaning away from the mouse, without cocos so it can be tested on its own. Where it wants to be
// comes only from where the mouse is next to its home (never from where it already went, so it can't drift or run
// away); a damped spring takes it there in fixed small steps, so it moves the same at any frame rate
class DodgeSpring
{
public:
  struct Vec
  {
    float x = 0;
    float y = 0;
  };

  struct Params
  {
    // Points: the mouse pushes inside this distance from the home, by up to `maxOffset`
    float radius = 55;
    float maxOffset = 11;
    // The push up and down, as a share of the sideways one
    float vertical = 0.35f;
    // Half the button's size: once the mouse is over the button's home, the button never leaves it, so a click
    // aimed at it lands. 0: no limit
    float halfWidth = 0;
    float halfHeight = 0;
    // How far inside the button's edge the mouse stays
    float margin = 3;
    float stiffness = 170;
    // 1: no overshoot; less springs past the place a little
    float damping = 0.55f;
  };

  // The integration step, whatever the frame rate
  static constexpr double STEP = 1.0 / 240;
  // Closer than this to the home, the push keeps its last direction: no jitter around the middle
  static constexpr float DEAD_ZONE = 2;
  // A frame longer than this (a stutter) moves the spring by this much only
  static constexpr double LONGEST_FRAME = 0.1;

  // Where the button wants to be, from the mouse's offset to its home; none: home
  Vec target(Vec const* mouseFromHome, Params const& params)
  {
    if (!mouseFromHome || params.radius <= 0 || params.maxOffset <= 0)
      return {};
    auto distance = std::hypot(mouseFromHome->x, mouseFromHome->y);
    if (distance >= params.radius)
      return {};
    if (distance > DEAD_ZONE)
      m_direction = {-mouseFromHome->x / distance, -mouseFromHome->y / distance};
    // The closer, the stronger, easing in at the edge of the radius
    auto closeness = 1.f - distance / params.radius;
    auto push = params.maxOffset * closeness * closeness * (3.f - 2.f * closeness);
    Vec wanted = clamp({m_direction.x * push, m_direction.y * push * params.vertical}, params.maxOffset);
    // The mouse over the home: the button leans, but stays under it
    auto insideX = params.halfWidth > params.margin && std::abs(mouseFromHome->x) <= params.halfWidth;
    auto insideY = params.halfHeight > params.margin && std::abs(mouseFromHome->y) <= params.halfHeight;
    if (insideX && insideY)
    {
      auto reachX = params.halfWidth - params.margin;
      auto reachY = params.halfHeight - params.margin;
      wanted.x = std::clamp(wanted.x, mouseFromHome->x - reachX, mouseFromHome->x + reachX);
      wanted.y = std::clamp(wanted.y, mouseFromHome->y - reachY, mouseFromHome->y + reachY);
    }
    return wanted;
  }

  // Moves the spring towards `target` by dt seconds of fixed steps
  void advance(double dt, Vec target, Params const& params)
  {
    m_pending += std::clamp(dt, 0.0, LONGEST_FRAME);
    auto stiffness = std::max(params.stiffness, 0.f);
    auto friction = 2.f * std::max(params.damping, 0.f) * std::sqrt(stiffness);
    while (m_pending >= STEP)
    {
      m_pending -= STEP;
      auto step = static_cast<float>(STEP);
      Vec force{stiffness * (target.x - m_offset.x) - friction * m_velocity.x,
                stiffness * (target.y - m_offset.y) - friction * m_velocity.y};
      m_velocity.x += force.x * step;
      m_velocity.y += force.y * step;
      m_offset.x += m_velocity.x * step;
      m_offset.y += m_velocity.y * step;
    }
    // A little past the most for the overshoot, never further
    m_offset = clamp(m_offset, params.maxOffset * 1.25f);
  }

  // While it's pressed: it stays where it is, so the click lands
  void hold()
  {
    m_velocity = {};
    m_pending = 0;
  }

  Vec offset() const { return m_offset; }
  Vec velocity() const { return m_velocity; }

  static Vec clamp(Vec v, float most)
  {
    auto length = std::hypot(v.x, v.y);
    if (length <= most || length <= 0)
      return v;
    return {v.x / length * most, v.y / length * most};
  }

private:
  Vec m_offset;
  Vec m_velocity;
  // Away from the mouse; to the right until the mouse has come from somewhere
  Vec m_direction{1, 0};
  double m_pending = 0;
};
