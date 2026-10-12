# UI integration requests

Concrete requests for shared files: exact path, setting/resource key or callback, intended behavior, and commit.
Keep 3D-specific calls out of screen code until the character API is agreed.

## UI-01 Statistics

No shared change needed. New sources (`src/statistics.cpp`, `src/statistics/MSStats.cpp`, `src/ui/MSScreen.cpp`,
`src/ui/MSScrollView.cpp`) are picked up by CMakeLists' `GLOB_RECURSE src/*.cpp`; no resources or mod.json
settings. The screen creates `MSMita::create()` like the achievements (current facade). Check in the combined
build: it compiles (only syntax-checked here), the numbers match GD's Stats window, GD STATS opens it.

## UI-02 Level completion

No shared change needed: `src/results.cpp`, `src/results/MSResults.cpp`, `src/ui/MSButtonMenu.cpp`,
`src/ui/MSValueRow.cpp` come in through the glob. It relies on geode.node-ids' EndLevelLayer IDs (installed v1.23.3:
`button-menu`, `retry-button`, `exit-button`, `edit-button`, `leaderboard-button`, `practice-retry-button`,
`summary-container` with `attempts-label` / `jumps-label` / `time-label` / `points-label`, `end-text`,
`complete-message`, `coin-N-background` / `coin-N-sprite`). Without `retry-button` and `exit-button` it leaves GD's
layer as it is. The right of the card is free for the later 3D reaction (no API assumed). To check in the combined
build: whether `coin-N-sprite` exists only for the coins collected (the card counts them so), the card's place on
4:3 and 21:9, and that the Win sound plays once.

