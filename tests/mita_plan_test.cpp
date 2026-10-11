// Mita's playback and paging, simulated: the same MitaPlan the mod uses, with a model of its page decoder (the
// latest request wins, one finished page waits, decoding takes a set time). Game frame rates, stutters and
// decoder speeds are varied; the invariants are checked on every frame
#include "MitaPlan.hpp"

#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>

namespace
{
  constexpr int FRAMES = 127;
  constexpr int ATLASES = 22;
  constexpr double FPS = 24;

  int atlasOf(int frame) { return frame / 6; }

  int failures = 0;

  void check(bool ok, std::string const& what)
  {
    if (ok)
      return;
    failures++;
    std::printf("  FAIL: %s\n", what.c_str());
  }

  struct Decoder
  {
    double latency = 0;
    int requested = -1;
    int decoding = -1;
    double done = 0;
    int ready = -1;

    bool busyWith(int atlas) const { return requested == atlas || decoding == atlas || ready == atlas; }

    void tick(double now)
    {
      if (decoding >= 0 && now >= done)
      {
        ready = decoding;
        decoding = -1;
        measured = latency;
      }
      if (decoding < 0 && requested >= 0)
      {
        decoding = requested;
        requested = -1;
        done = now + latency;
      }
    }

    int take()
    {
      auto page = ready;
      ready = -1;
      return page;
    }

    // What the mod measures: the time a page takes, known once one finished
    double measured = 0;
  };

  struct Result
  {
    int stalls = 0;
    int seamStalls = 0;
    size_t maxResident = 0;
    int uploads = 0;
    int lastShown = -1;
    double elapsed = 0;
  };

  // dt for frame `i`: the game's frame time, with a stutter every `stutterEvery` frames
  Result simulate(std::string const& name, double gameFps, double latency, int stutterEvery, double stutter,
                  double seconds)
  {
    MitaPlan plan(ATLASES);
    Decoder decoder;
    decoder.latency = latency;
    Result result;
    double now = 0;
    auto started = false;
    int previous = -1;
    for (int i = 0; now < seconds; i++)
    {
      auto dt = (stutterEvery > 0 && i % stutterEvery == stutterEvery - 1) ? stutter : 1.0 / gameFps;
      now += dt;
      result.elapsed = MitaPlan::advance(result.elapsed, dt, FPS, FRAMES);
      auto index = MitaPlan::frameAt(result.elapsed, FPS, FRAMES);
      auto atlas = atlasOf(index);

      decoder.tick(now);
      // As the mod does: the page for now first, then the finished one
      if (plan.isResident(atlas))
        plan.showed(atlas);
      if (decoder.ready >= 0)
      {
        auto use = plan.decide(decoder.ready, atlas);
        if (use != MitaPlan::Use::Wait)
        {
          auto decoded = decoder.take();
          if (use == MitaPlan::Use::Upload)
          {
            plan.makeRoom(atlas);
            check(plan.resident().size() < 2, name + ": room made for the upload");
            plan.uploaded(decoded);
            result.uploads++;
          }
        }
      }
      result.maxResident = std::max(result.maxResident, plan.resident().size());
      check(plan.resident().size() <= 2, name + ": at most two pages on the GPU");

      if (plan.isResident(atlas))
      {
        plan.showed(atlas);
        result.lastShown = index;
        started = true;
      }
      else if (started)
      {
        result.stalls++;
        if (previous == FRAMES - 1 || index == 0)
          result.seamStalls++;
      }
      if (plan.shown() >= 0)
        check(plan.isResident(plan.shown()), name + ": the page shown is on the GPU");
      // As the mod does: where playback will be once a load ends, with its margin
      auto later = MitaPlan::advance(result.elapsed, decoder.measured * 1.5, FPS, FRAMES);
      auto predicted = atlasOf(MitaPlan::frameAt(later, FPS, FRAMES));
      if (auto request = plan.toRequest(atlas, predicted); request >= 0 && !decoder.busyWith(request))
        decoder.requested = request;
      previous = index;
    }
    std::printf("%-34s stalls %4d (seam %d), uploads %4d, most pages on GPU %zu\n", name.c_str(), result.stalls,
                result.seamStalls, result.uploads, result.maxResident);
    return result;
  }
} // namespace

int main()
{
  // The frame from the time: 24 a second whatever the game's rate
  check(MitaPlan::frameAt(0, FPS, FRAMES) == 0, "frame 0 at the start");
  check(MitaPlan::frameAt(1.0, FPS, FRAMES) == 24, "frame 24 after a second");
  check(MitaPlan::frameAt(126.0 / 24 + 1e-9, FPS, FRAMES) == 126, "the last frame");
  check(MitaPlan::frameAt(127.0 / 24 + 1e-9, FPS, FRAMES) == 0, "back to 0 after 127 frames (no ping-pong)");
  check(std::abs(MitaPlan::advance(5.0, 0.5, FPS, FRAMES) - (5.5 - 127.0 / 24)) < 1e-9, "the loop wraps");

  // The same time at 30, 60 and 144 fps: the same frame, no drift from adding many small steps. Started half a
  // frame in, so a drift of half a frame (21 ms) would show
  for (double fps : {30.0, 60.0, 144.0, 240.0})
  {
    double start = 0.5 / FPS;
    double elapsed = start;
    for (int i = 0; i < static_cast<int>(fps * 60); i++)
      elapsed = MitaPlan::advance(elapsed, 1.0 / fps, FPS, FRAMES);
    auto expected = MitaPlan::frameAt(std::fmod(start + 60.0, 127.0 / 24), FPS, FRAMES);
    check(MitaPlan::frameAt(elapsed, FPS, FRAMES) == expected,
          "after 60 s at " + std::to_string(static_cast<int>(fps)) + " fps the frame is the one for 60 s");
  }

  // A stutter jumps to the frame for now, not through the ones missed
  {
    auto elapsed = MitaPlan::advance(0, 1.0, FPS, FRAMES);
    check(MitaPlan::frameAt(elapsed, FPS, FRAMES) == 24, "one 1 s frame lands on frame 24");
  }

  // Decoding a page at a usual speed (well under the 0.25 s a page is shown): no stall, the seam included
  for (double fps : {30.0, 60.0, 144.0})
  {
    auto result = simulate(std::to_string(static_cast<int>(fps)) + " fps, 60 ms decode", fps, 0.06, 0, 0, 60);
    check(result.stalls == 0, "no stall at " + std::to_string(static_cast<int>(fps)) + " fps with a 60 ms decode");
    check(result.seamStalls == 0, "the 126 -> 0 seam is decoded ahead");
    check(result.maxResident <= 2, "two pages at most");
  }

  // Stutters of half a second: the frame jumps, pages are skipped, still never three on the GPU
  {
    auto result = simulate("60 fps, 0.5 s stutters, 60 ms decode", 60, 0.06, 90, 0.5, 60);
    check(result.maxResident <= 2, "two pages at most with stutters");
    auto expected = MitaPlan::frameAt(result.elapsed, FPS, FRAMES);
    check(result.lastShown == expected || result.stalls > 0, "after stutters it shows the frame for now");
  }

  // A decoder slower than a page (0.4 s against 0.25 s): it can't keep up, so frames stall (the last one stays),
  // but it looks ahead by its measured time and keeps showing the page for now as they come; never three pages
  {
    auto result = simulate("60 fps, 400 ms decode (too slow)", 60, 0.4, 0, 0, 30);
    check(result.maxResident <= 2, "two pages at most with a slow decoder");
    check(result.uploads > 20, "a slow decoder still shows pages as they come");
    check(result.stalls > 0, "a too slow decoder stalls (the last frame stays)");
  }

  // A decoder a bit slower than the one-frame last page (100 ms against 42 ms): the seam is decoded ahead
  {
    auto result = simulate("60 fps, 100 ms decode", 60, 0.1, 0, 0, 60);
    check(result.seamStalls == 0, "the seam doesn't stall with a 100 ms decode");
    check(result.stalls == 0, "no stall with a 100 ms decode");
  }

  // Leaving the menu frees everything
  {
    MitaPlan plan(ATLASES);
    plan.uploaded(3);
    plan.showed(3);
    plan.uploaded(4);
    auto freed = plan.clear();
    check(freed.size() == 2 && plan.resident().empty() && plan.shown() == -1, "clear frees every page");
  }

  if (failures)
  {
    std::printf("%d check(s) failed\n", failures);
    return EXIT_FAILURE;
  }
  std::printf("All checks passed\n");
  return EXIT_SUCCESS;
}
