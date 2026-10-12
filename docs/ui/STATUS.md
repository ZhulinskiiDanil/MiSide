# UI status

Lane: `D:/Dev/MiSide-ui`, branch `feature/miside-ui`. Builds, installs and in-game checks are not run in this
stage: "done" means finished code with its integration requests recorded, not verified behavior in the combined
build. Syntax is checked with `clang++ -fsyntax-only` and the main workspace's compile flags (no objects written).

3D signal (`3D_STATUS: READY_FOR_INTEGRATION` in `D:/Dev/MiSide-3d/docs/3d/INTEGRATION.md`): not present when
checked before UI-01.

| Task | Status | Result | Commit |
| --- | --- | --- | --- |
| UI-01 Statistics | DONE | `src/statistics.cpp`, `src/statistics/MSStats.*`: MiSide screen over the menu with GD's own stats (StatsLayer's keys and order, GameStatsManager values, StatsCell titles), groups, scrolling, GD STATS button to the original window. Shared base `src/ui/MSScreen.*`, `src/ui/MSScrollView.*` for the next screens. Syntax checked; not built, not run | a48f215 |
| UI-02 Level completion | DONE | `src/results.cpp`, `src/results/MSResults.*`: EndLevelLayer's frame, chains and labels hidden behind a MiSide card (title, level, GD's end text and message, GD's own label values, coins from GD's coin nodes, buttons pressing GD's own); mods' buttons and GD's reward effects kept; right side left for Mita. Shared `src/ui/MSButtonMenu.*`, `src/ui/MSValueRow.*`. Syntax checked; not built, not run | 88bc459 |
| UI-03 Appearance settings | DONE | `src/settings.cpp`, `src/settings/MSSettings.hpp`: APPEARANCE screen (column entry after SETTINGS in `src/main.cpp`) with the mod's existing Geode settings in groups (choices with values, parallax slider with its number and reset) and GD's music/effects volumes; RESET through Geode's `Setting::reset()`; the menu remade on leaving when what it shows changed. `MSPauseColumn` gained choice rows, group headers and a slider value format. Missing settings (particles, transitions, UI sound volume) requested in INTEGRATION.md. Syntax checked; not built, not run | 25743da |
| UI-04 Official levels polish | DONE | Inspected first: the floppy already had long-name fitting, coins, normal/practice progress and the coins-needed lock, Up/Down/Enter worked. Added in `src/levels.cpp`, `src/levels/MSLevelList.*`, `src/ui/MSMenuItem.*`: per-row lock (GD's own coin check) or best %, names fitted around it (`reserveRight`, row accessory fading with the row), the chosen level and scroll kept in the mod's saved values (GD always opens page 0 from the menu), the played level chosen after a level. LevelPreview untouched. Syntax checked; not built, not run | a250b13 |
| UI-05 Pause usability | DONE | `src/pause/MSPauseColumn.*`, `src/pause.cpp`: the row chosen before the quit confirmation or More is chosen again when the column shows (no new shine; the mouse still takes over), the wheel over a slider row moves it by the keys' step, More's BACK is a separate exit row with its close sound. Blur, OpenGL state, ambient, Stress and third-party nodes untouched; the vanilla-pause route kept. Syntax checked; not built, not run | (this commit) |
| UI-06 Dialogs and layout | TODO | — | — |
| UI-07 Documentation and credits | TODO | — | — |
