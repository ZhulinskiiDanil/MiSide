# UI integration requests

Concrete requests for shared files: exact path, setting/resource key or callback, intended behavior, and commit.
Keep 3D-specific calls out of screen code until the character API is agreed.

## UI-01 Statistics

No shared change needed. New sources (`src/statistics.cpp`, `src/statistics/MSStats.cpp`, `src/ui/MSScreen.cpp`,
`src/ui/MSScrollView.cpp`) are picked up by CMakeLists' `GLOB_RECURSE src/*.cpp`; no resources or mod.json
settings. The screen creates `MSMita::create()` like the achievements (current facade). Check in the combined
build: it compiles (only syntax-checked here), the numbers match GD's Stats window, GD STATS opens it.
