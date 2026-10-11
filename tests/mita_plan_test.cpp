// Mita's playback and paging, simulated: the same MitaPlan the mod uses, with a model of its page decoder (the
// latest request wins, one finished page waits, decoding takes a set time). Game frame rates, stutters and
// decoder speeds are varied; the invariants are checked on every frame
#include "MitaPlan.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>

namespace
{
  // The close-up set: two frames a page (83 ms of her), the last page one frame
  constexpr int FRAMES = 127;
  constexpr int ATLASES = 64;
  constexpr double FPS = 24;

  int atlasOf(int frame) { return frame / 2; }

  int failures = 0;

  void check(bool ok, std::string const& what)
  {
    if (ok)
      return;
    failures++;
    std::printf("  FAIL: %s\n", what.c_str());
  }

  // The mod's page loader: it decodes the pages asked for one after another, nearest first (a newer list replaces
  // the pages not started yet), and stops while two decoded pages wait to be taken
  struct Decoder
  {
    static constexpr size_t MAX_READY = 2;
    double latency = 0;
    std::vector<int> wanted;
    int decoding = -1;
    double done = 0;
    double idleSince = 0;
    bool stalled = true;
    std::vector<int> ready;
    // What the mod measures: the time a page takes, known once one finished
    double measured = 0;

    bool busyWith(int atlas) const
    {
      return decoding == atlas || std::find(ready.begin(), ready.end(), atlas) != ready.end();
    }

    void want(std::vector<int> const& pages, double now)
    {
      wanted.clear();
      for (auto page : pages)
      {
        if (!busyWith(page))
          wanted.push_back(page);
      }
      run(now);
    }

    // Finishes what's done by now and starts the next ones, each as soon as the one before ended
    void run(double now)
    {
      while (true)
      {
        if (decoding >= 0)
        {
          if (done > now)
            return;
          ready.push_back(decoding);
          decoding = -1;
          idleSince = done;
          measured = latency;
        }
        if (wanted.empty() || ready.size() >= MAX_READY)
        {
          // Idle until something changes: a request or a page taken, at the earliest now
          stalled = true;
          return;
        }
        if (stalled)
          idleSince = std::max(idleSince, now);
        stalled = false;
        decoding = wanted.front();
        wanted.erase(wanted.begin());
        done = idleSince + latency;
      }
    }

    void take(int atlas, double now)
    {
      std::erase(ready, atlas);
      run(now);
    }
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
      // As the mod does: her clock starts with her first frame shown
      if (started)
        result.elapsed = MitaPlan::advance(result.elapsed, dt, FPS, FRAMES);
      auto index = MitaPlan::frameAt(result.elapsed, FPS, FRAMES);
      auto atlas = atlasOf(index);

      decoder.run(now);
      // As the mod does: the page for now first, then every finished one
      if (plan.isResident(atlas))
        plan.showed(atlas);
      for (auto decoded : std::vector<int>(decoder.ready))
      {
        auto use = plan.decide(decoded, atlas);
        if (use == MitaPlan::Use::Wait)
          continue;
        if (use == MitaPlan::Use::Upload)
        {
          plan.makeRoom(atlas);
          check(plan.resident().size() < MitaPlan::RESIDENT, name + ": room made for the upload");
          plan.uploaded(decoded);
          result.uploads++;
        }
        decoder.take(decoded, now);
      }
      check(decoder.ready.size() <= Decoder::MAX_READY, name + ": at most two decoded pages wait");
      result.maxResident = std::max(result.maxResident, plan.resident().size());
      check(plan.resident().size() <= MitaPlan::RESIDENT, name + ": at most three pages on the GPU");

      if (plan.isResident(atlas))
      {
        plan.showed(atlas);
        result.lastShown = index;
        started = true;
      }
      else if (started)
      {
        result.stalls++;
        if (std::getenv("MITA_TRACE"))
          std::printf("    stall at %.3f s, frame %d, page %d\n", now, index, atlas);
        if (previous == FRAMES - 1 || index == 0)
          result.seamStalls++;
      }
      if (plan.shown() >= 0)
        check(plan.isResident(plan.shown()), name + ": the page shown is on the GPU");
      // As the mod does: where playback will be once a load ends, with its margin
      auto later = MitaPlan::advance(result.elapsed, decoder.measured * 1.5, FPS, FRAMES);
      auto predicted = atlasOf(MitaPlan::frameAt(later, FPS, FRAMES));
      decoder.want(plan.toRequest(atlas, predicted), now);
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

  // The pages of the frames to check: 1 -> 2 changes page, 126 is the last page's only frame
  check(atlasOf(0) == 0 && atlasOf(1) == 0 && atlasOf(2) == 1 && atlasOf(63) == 31 && atlasOf(126) == 63,
        "frames 0, 1, 2, 63, 126 on pages 0, 0, 1, 31, 63");

  // The window around the seam: the last page's next two are the first two
  {
    MitaPlan plan(ATLASES);
    check(plan.inWindow(63, 62) && plan.inWindow(0, 62) && !plan.inWindow(1, 62), "page 62: 62, 63 and 0 kept");
    check(plan.inWindow(0, 63) && plan.inWindow(1, 63) && !plan.inWindow(62, 63), "page 63: 63, 0 and 1 kept");
    for (int atlas : {62, 63, 0})
      plan.uploaded(atlas);
    auto freed = plan.showed(63);
    check(freed.size() == 1 && freed[0] == 62, "showing 63 frees 62 (after the sprite is on 63)");
    auto asked = plan.toRequest(63, 63);
    check(asked.size() == 2 && asked[0] == 1 && asked[1] == 2, "then page 1 is asked for, and 2 to wait");
  }

  // Decoding a page at the measured speed (17 ms) and slower ones, well under the 83 ms a page is shown: no
  // stall, the seam included
  for (double latency : {0.017, 0.04, 0.06})
  {
    for (double fps : {30.0, 60.0, 144.0})
    {
      auto label = std::to_string(static_cast<int>(fps)) + " fps, " + std::to_string(static_cast<int>(latency * 1000)) +
                   " ms decode";
      auto result = simulate(label, fps, latency, 0, 0, 60);
      check(result.stalls == 0, "no stall at " + label);
      check(result.seamStalls == 0, "the 126 -> 0 seam is decoded ahead at " + label);
      check(result.maxResident <= MitaPlan::RESIDENT, "three pages at most at " + label);
    }
  }

  // Stutters of half a second: the frame jumps, pages are skipped, still never four on the GPU
  {
    auto result = simulate("60 fps, 0.5 s stutters, 17 ms decode", 60, 0.017, 90, 0.5, 60);
    check(result.maxResident <= MitaPlan::RESIDENT, "three pages at most with stutters");
    auto expected = MitaPlan::frameAt(result.elapsed, FPS, FRAMES);
    check(result.lastShown == expected || result.stalls > 0, "after stutters it shows the frame for now");
  }

  // A decoder slower than a page (150 ms against 83 ms): it can't keep up, so frames stall (the last one stays),
  // but it looks ahead by its measured time and keeps showing the page for now as they come; never four pages
  {
    auto result = simulate("60 fps, 150 ms decode (too slow)", 60, 0.15, 0, 0, 30);
    check(result.maxResident <= MitaPlan::RESIDENT, "three pages at most with a slow decoder");
    check(result.uploads > 100, "a slow decoder still shows pages as they come");
    check(result.stalls > 0, "a too slow decoder stalls (the last frame stays)");
  }

  // A decoder slower than the one-frame last page (70 ms against 42 ms): the seam is decoded ahead
  {
    auto result = simulate("60 fps, 70 ms decode", 60, 0.07, 0, 0, 60);
    check(result.seamStalls == 0, "the seam doesn't stall with a 70 ms decode");
    check(result.stalls == 0, "no stall with a 70 ms decode");
  }

  // Leaving the menu frees everything
  {
    MitaPlan plan(ATLASES);
    plan.uploaded(3);
    plan.showed(3);
    plan.uploaded(4);
    plan.uploaded(5);
    auto freed = plan.clear();
    check(freed.size() == 3 && plan.resident().empty() && plan.shown() == -1, "clear frees every page");
  }

  if (failures)
  {
    std::printf("%d check(s) failed\n", failures);
    return EXIT_FAILURE;
  }
  std::printf("All checks passed\n");
  return EXIT_SUCCESS;
}
