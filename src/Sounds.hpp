#pragma once

// MiSide's sounds outside the mod's own menus: GD's settings toggles and pages, the icon kit's tabs and icons, an
// achievement, a level's end. And how close the paused attempt is to the personal best, for the pause's Stress
namespace sounds
{
  // 0: not near it (or it doesn't count: practice, a start position, a platformer, a completed level, a best under
  // `pause-stress-min-best`); 1: within `pause-stress-window` points under the best at the attempt's start;
  // `pause-stress-boost`: past it
  float stressLevel();
} // namespace sounds
