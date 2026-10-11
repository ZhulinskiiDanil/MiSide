// The shine's pass, simulated: from hidden on the left to gone on the right in the set time at any frame rate,
// one pass per start, a restart from the beginning, a fade when the mouse leaves
#include "ShinePass.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

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

  constexpr float DURATION = 0.2f;
  constexpr float WIDTH = 0.2f;
  constexpr float FADE_OUT = 0.12f;
} // namespace

int main()
{
  // From fully hidden to fully gone
  {
    ShinePass pass;
    pass.start();
    check(std::abs(pass.center(DURATION, WIDTH) - (-WIDTH / 2)) < 1e-6f, "the band starts fully left of the label");
    pass.advance(DURATION, DURATION, FADE_OUT);
    check(std::abs(pass.center(DURATION, WIDTH) - (1 + WIDTH / 2)) < 1e-6f, "the band ends fully right of it");
    check(!pass.active(), "one pass, then it stops");
  }

  // The same time at 30, 60, 144 and 240 fps: it ends on the frame where the time is up, the middle where it
  // should be halfway
  for (float fps : {30.f, 60.f, 144.f, 240.f})
  {
    ShinePass pass;
    pass.start();
    int frames = 0;
    float half = -1;
    while (pass.active() && frames < 1000)
    {
      pass.advance(1 / fps, DURATION, FADE_OUT);
      frames++;
      if (half < 0 && frames / fps >= DURATION / 2 - 1e-6f)
        half = pass.center(DURATION, WIDTH);
    }
    auto seconds = frames / fps;
    std::printf("%3d fps: %d frames, %.3f s, middle at half time %.3f\n", static_cast<int>(fps), frames, seconds, half);
    check(seconds >= DURATION - 1e-4f && seconds < DURATION + 1 / fps + 1e-4f,
          "the pass takes 0.2 s at " + std::to_string(static_cast<int>(fps)) + " fps");
    check(std::abs(half - 0.5f) < (1 + WIDTH) / (fps * DURATION) + 1e-3f, "halfway through at half the time");
  }

  // A restart begins again from the left; nothing carries over
  {
    ShinePass pass;
    pass.start();
    pass.advance(DURATION * 0.7f, DURATION, FADE_OUT);
    pass.leave();
    pass.start();
    check(pass.active() && pass.fade() == 1 && std::abs(pass.center(DURATION, WIDTH) + WIDTH / 2) < 1e-6f,
          "hovering again starts a new pass from the left");
  }

  // Leaving fades it out, and it ends
  {
    ShinePass pass;
    pass.start();
    pass.advance(0.05f, 1.f, FADE_OUT);
    pass.leave();
    pass.advance(FADE_OUT / 2, 1.f, FADE_OUT);
    check(pass.active() && std::abs(pass.fade() - 0.5f) < 1e-4f, "half faded halfway through the fade");
    pass.advance(FADE_OUT, 1.f, FADE_OUT);
    check(!pass.active() && pass.fade() == 0, "faded out and stopped");
  }

  // Never started: nothing
  {
    ShinePass pass;
    pass.advance(1, DURATION, FADE_OUT);
    check(!pass.active(), "no pass without a start");
  }

  if (failures)
  {
    std::printf("%d check(s) failed\n", failures);
    return EXIT_FAILURE;
  }
  std::printf("All checks passed\n");
  return EXIT_SUCCESS;
}
