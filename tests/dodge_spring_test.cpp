// The Exit button's spring, simulated: the mouse coming close, sitting on the middle, leaving; frame rates from
// 30 to 240 and stutters. The checks: the same motion at any frame rate, bounded, no jitter, back home, still
// under a mouse that aims at it, and still while pressed
#include "DodgeSpring.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

namespace
{
  int failures = 0;

  void check(bool ok, std::string const& what)
  {
    if (ok)
      return;
    failures++;
    std::printf("  FAIL: %s\n", what.c_str());
  }

  using Vec = DodgeSpring::Vec;
  // The button: 59 x 15 points
  constexpr float HALF_W = 29.5f;
  constexpr float HALF_H = 7.5f;
  DodgeSpring::Params const PARAMS = [] {
    DodgeSpring::Params params;
    params.halfWidth = HALF_W;
    params.halfHeight = HALF_H;
    return params;
  }();

  // The mouse over time, relative to the home: false when it's away
  using Path = std::function<bool(double t, Vec& mouse)>;

  struct Run
  {
    std::vector<Vec> samples;
    float most = 0;
    float biggestJump = 0;
  };

  // The input for each frame is where the mouse was at the frame's start
  Run simulate(Path const& path, double fps, double seconds, int stutterEvery = 0)
  {
    DodgeSpring spring;
    Run run;
    double t = 0;
    Vec previous;
    for (int i = 0; t < seconds; i++)
    {
      auto dt = (stutterEvery > 0 && i % stutterEvery == stutterEvery - 1) ? 0.3 : 1.0 / fps;
      Vec mouse;
      auto has = path(t, mouse);
      spring.advance(dt, spring.target(has ? &mouse : nullptr, PARAMS), PARAMS);
      t += dt;
      auto offset = spring.offset();
      run.most = std::max(run.most, std::hypot(offset.x, offset.y));
      run.biggestJump = std::max(run.biggestJump, std::hypot(offset.x - previous.x, offset.y - previous.y));
      previous = offset;
      run.samples.push_back(offset);
    }
    return run;
  }

  // Beside the button, then on its middle, then gone: at moments that every frame rate here has a frame on
  bool steps(double t, Vec& mouse)
  {
    if (t < 0.2 - 1e-9)
      return false;
    if (t < 0.8 - 1e-9)
    {
      mouse = {40.f, 4.f};
      return true;
    }
    if (t < 1.6 - 1e-9)
    {
      mouse = {0.f, 0.f};
      return true;
    }
    return false;
  }

  // Coming from the right along the button's row, resting on the middle, then gone
  bool approach(double t, Vec& mouse)
  {
    if (t < 0.2)
      return false;
    if (t < 1.2)
    {
      mouse = {static_cast<float>(80 - (t - 0.2) * 80), 0.f};
      return true;
    }
    if (t < 2.2)
    {
      mouse = {0.f, 0.f};
      return true;
    }
    return false;
  }
} // namespace

int main()
{
  // The same input at 30, 60, 120 and 240 fps: the same motion, compared every 1/30 s
  {
    auto every30 = [](double fps) {
      auto run = simulate(steps, fps, 3);
      std::vector<Vec> out;
      auto every = static_cast<int>(fps / 30);
      for (size_t i = every - 1; i < run.samples.size(); i += every)
        out.push_back(run.samples[i]);
      return out;
    };
    auto reference = every30(240);
    for (double fps : {30.0, 60.0, 120.0})
    {
      auto run = every30(fps);
      auto worst = 0.f;
      for (size_t i = 0; i < std::min(run.size(), reference.size()); i++)
        worst = std::max(worst, std::hypot(run[i].x - reference[i].x, run[i].y - reference[i].y));
      std::printf("%3d fps vs 240 fps, same input: %.4f pt apart at most\n", static_cast<int>(fps), worst);
      check(worst < 0.01f, "the same motion at " + std::to_string(static_cast<int>(fps)) + " fps");
    }
  }

  // A mouse that moves every frame, at 30 to 240 fps: bounded, small moves a frame, back home after
  for (double fps : {30.0, 60.0, 144.0, 240.0})
  {
    auto run = simulate(approach, fps, 4);
    std::printf("%3d fps, moving mouse: furthest %.2f pt, biggest move in a frame %.2f pt\n", static_cast<int>(fps),
                run.most, run.biggestJump);
    check(run.most <= PARAMS.maxOffset * 1.25f + 1e-3f, "never further than the most plus the overshoot");
    check(run.biggestJump < 60 * PARAMS.maxOffset / fps + 0.5f, "no jump in one frame");
    auto end = run.samples.back();
    check(std::hypot(end.x, end.y) < 0.05f, "back home 1.8 s after the mouse leaves");
  }

  // The mouse resting on the middle: it settles without jitter, still under the mouse
  {
    DodgeSpring spring;
    Vec mouse{0.f, 0.f};
    float lastMove = 0;
    for (int i = 0; i < 600; i++)
    {
      auto before = spring.offset();
      spring.advance(1.0 / 60, spring.target(&mouse, PARAMS), PARAMS);
      if (i > 300)
        lastMove = std::max(lastMove, std::hypot(spring.offset().x - before.x, spring.offset().y - before.y));
    }
    auto offset = spring.offset();
    std::printf("mouse on the middle: settles at (%.2f, %.2f), still moving %.4f pt a frame\n", offset.x, offset.y,
                lastMove);
    check(lastMove < 0.01f, "no jitter with the mouse on the middle");
    check(std::abs(offset.x) < HALF_W && std::abs(offset.y) < HALF_H, "the button stays under a mouse on its home");
  }

  // The mouse anywhere on the button's home: where the button settles, it's still under the mouse
  {
    auto reachable = true;
    auto missedX = 0.f;
    auto missedY = 0.f;
    for (float x = -HALF_W + 0.5f; x <= HALF_W - 0.5f; x += 1)
      for (float y = -HALF_H + 0.5f; y <= HALF_H - 0.5f; y += 0.5f)
      {
        DodgeSpring spring;
        Vec mouse{x, y};
        for (int i = 0; i < 240; i++)
          spring.advance(1.0 / 60, spring.target(&mouse, PARAMS), PARAMS);
        auto offset = spring.offset();
        if (std::abs(x - offset.x) > HALF_W || std::abs(y - offset.y) > HALF_H)
        {
          reachable = false;
          missedX = x;
          missedY = y;
        }
      }
    if (!reachable)
      std::printf("  the mouse at (%.1f, %.1f) ends off the button\n", missedX, missedY);
    check(reachable, "aiming at any point of the button's home lands on the button");
  }

  // Stutters of 0.3 s: still bounded, finite
  {
    auto run = simulate(approach, 60, 4, 20);
    check(run.most <= PARAMS.maxOffset * 1.25f + 1e-3f, "bounded with stutters");
    check(std::isfinite(run.samples.back().x) && std::isfinite(run.samples.back().y), "finite with stutters");
  }

  // Held while pressed: no motion
  {
    DodgeSpring spring;
    Vec mouse{10.f, 0.f};
    for (int i = 0; i < 30; i++)
      spring.advance(1.0 / 60, spring.target(&mouse, PARAMS), PARAMS);
    auto pressedAt = spring.offset();
    spring.hold();
    check(spring.offset().x == pressedAt.x && spring.offset().y == pressedAt.y, "still while pressed");
  }

  if (failures)
  {
    std::printf("%d check(s) failed\n", failures);
    return EXIT_FAILURE;
  }
  std::printf("All checks passed\n");
  return EXIT_SUCCESS;
}
