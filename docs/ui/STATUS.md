# UI status

Lane: `D:/Dev/MiSide-ui`, branch `feature/miside-ui`. Builds, installs and in-game checks are not run in this
stage: "done" means finished code with its integration requests recorded, not verified behavior in the combined
build. Syntax is checked with `clang++ -fsyntax-only` and the main workspace's compile flags (no objects written).

3D signal (`3D_STATUS: READY_FOR_INTEGRATION` in `D:/Dev/MiSide-3d/docs/3d/INTEGRATION.md`): not present when
checked before UI-01.

| Task | Status | Result | Commit |
| --- | --- | --- | --- |
| UI-01 Statistics | DONE | `src/statistics.cpp`, `src/statistics/MSStats.*`: MiSide screen over the menu with GD's own stats (StatsLayer's keys and order, GameStatsManager values, StatsCell titles), groups, scrolling, GD STATS button to the original window. Shared base `src/ui/MSScreen.*`, `src/ui/MSScrollView.*` for the next screens. Syntax checked; not built, not run | (this commit) |
| UI-02 Level completion | TODO | — | — |
| UI-03 Appearance settings | TODO | — | — |
| UI-04 Official levels polish | TODO | — | — |
| UI-05 Pause usability | TODO | — | — |
| UI-06 Dialogs and layout | TODO | — | — |
| UI-07 Documentation and credits | TODO | — | — |
