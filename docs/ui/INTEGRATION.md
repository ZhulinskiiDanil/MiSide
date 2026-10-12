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

## UI-03 Appearance settings

Works with the current mod.json settings only (`menu-language`, `show-title`, `show-intro`, `confirm-level-exit`,
`parallax-strength`, `vanilla-menu`) and GD's own volumes; no new storage. New sources come in through the glob.

Request (mod.json, integration zone): settings the page would show if they existed, each mapped onto an existing
Theme value so the theme stays the one source of defaults:

| Key | Type, default, range | Theme value it would drive | Where it's read |
| --- | --- | --- | --- |
| `show-particles` | bool, true | `particle-count` (0 when off) | `src/ui/MSParticles.cpp` init |
| `transition-speed` | float, 1.0, 0.5–2.0 | divides `fade-duration`, `fade-back-duration`, `pause-duration`, `hover-duration` | `Theme::reload()` after the theme file |
| `ui-sound-volume` | int, 100, 0–100 | `sound-volume` (/100) | `ms::playSound` |

When they're registered, the UI lane adds the rows to `src/settings.cpp` (an EFFECTS group) and applies them in
`Theme::reload()`; until then they stay theme.json keys. To check in the combined build: the column entry fits above
EXIT, and `MenuLayer::scene(false)` through `CCTransitionFade` after a change keeps the menu music playing.

## UI-04 Official levels polish

No shared change. The choice and scroll are kept in the mod's saved values (`levels-chosen-level`, `levels-offset`
in saved.json), not settings. `LevelPreview.*` and its API untouched. To check in the combined build: the row
accessory's place on long names (Theory of Everything 2, Geometrical Dominator) and that a locked demon shows the
lock while the floppy says how many coins are needed.

## UI-05 Pause usability

No shared change; `MSPauseBackdrop.*` untouched. `MSPauseColumn` now takes the mouse wheel (`setMouseEnabled`): to
check in the combined build that it doesn't take the wheel from another mod's scrolling list shown in More, and that
the restored highlight after the confirmation looks right with a controller.

## UI-06 Dialogs and layout

No shared change. `MSMenuItem::Style::confirm()` now fits its text, which also applies to the main menu's quit
confirmation (YES / NO, short: no visible change expected). To check in the combined build on 4:3, 16:9 and 21:9:
the column rows' labels against sliders and values, the results card's width, and RESET's confirmation in APPEARANCE.

