#pragma once

#include <algorithm>
#include <cmath>

// Mita's greeting at the game's first menu, as one clock: a purple-pink veil over her lifts as she shows, she greets
// from afar while the camera comes up to her close-up (faster towards the end, slowing down into it), then her
// greeting gives way to her menu pose, the light around her settles and the interface comes in. Every curve starts
// and ends still, so nothing jumps where one part hands over to the next. Plain math: no Geode, no cocos
namespace greeting
{
  // From the scene being shown: the veil lifts over these seconds
  inline constexpr float VeilStart = 0.15f;
  inline constexpr float VeilEnd = 1.75f;
  // Around the animation's end: its weight goes from 1 to 0 (the last frame held after it), the camera arrives
  inline constexpr float BlendBefore = 0.35f;
  inline constexpr float BlendAfter = 0.65f;
  // The light and the edges settle to the menu's look
  inline constexpr float AuraBefore = 0.6f;
  inline constexpr float AuraAfter = 0.9f;
  // The interface starts coming in this long before the animation ends
  inline constexpr float InterfaceBefore = 0.f;
  // How much more of the camera's way is left for the end: progress = ease(t ^ CameraBias)
  inline constexpr float CameraBias = 1.6f;

  struct State
  {
    // Seconds into the animation (held at its end), and how much of it shows over her menu pose
    float animation = 0;
    float weight = 1;
    // 1: the far shot, 0: the menu's close-up
    float zoom = 1;
    // The veil's opacity and hers, 0..1
    float veil = 1;
    float model = 0;
    // 1: the greeting's light and edges, 0: the menu's
    float aura = 1;
    // The interface has been asked to come in
    bool showInterface = false;
    // All at the menu's values: the greeting is over
    bool done = false;
  };

  inline float clamp01(float value) { return std::clamp(value, 0.f, 1.f); }
  // Still at both ends
  inline float ease(float value)
  {
    auto u = clamp01(value);
    return u * u * (3 - 2 * u);
  }
  inline float easeSmoother(float value)
  {
    auto u = clamp01(value);
    return u * u * u * (u * (u * 6 - 15) + 10);
  }
  inline float between(float clock, float from, float to) { return to > from ? (clock - from) / (to - from) : 1.f; }

  inline float end(float duration) { return duration + std::max(BlendAfter, AuraAfter); }

  // `clock`: seconds since the scene was shown; `duration`: the animation's length
  inline State at(float clock, float duration)
  {
    State s;
    duration = std::max(duration, 0.f);
    clock = std::max(clock, 0.f);
    s.animation = std::min(clock, duration);
    s.weight = 1 - easeSmoother(between(clock, duration - BlendBefore, duration + BlendAfter));
    s.zoom = 1 - ease(std::pow(clamp01(clock / (duration + BlendAfter)), CameraBias));
    s.veil = 1 - ease(between(clock, VeilStart, VeilEnd));
    s.model = 1 - s.veil;
    s.aura = 1 - ease(between(clock, duration - AuraBefore, duration + AuraAfter));
    s.showInterface = clock >= duration - InterfaceBefore;
    s.done = clock >= end(duration);
    if (s.done)
    {
      s.weight = 0;
      s.zoom = 0;
      s.veil = 0;
      s.model = 1;
      s.aura = 0;
    }
    return s;
  }
} // namespace greeting
