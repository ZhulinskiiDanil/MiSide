# UI tasks for Claude

Work in D:/Dev/MiSide-ui. Achievements are already implemented. This is an ordered
backlog; finish one task before starting the next, and don't redo completed screens.
Use the existing MiSide palette, font, menu items, shine, transitions and sounds.
Keep all values based on actual GD data. Avoid full replacement of crowded screens.

## UI-01 — Statistics screen (first task)

Create src/statistics.cpp and src/statistics/. Present the player's existing GD
statistics as a readable MiSide page: title, compact groups and scrolling when
needed. Preserve the original stats entry/callback behavior. Reuse the existing
Mita facade if she fits; no 3D-specific assumptions. Include a way to open the
original stats UI if mod-added content cannot be represented. Do not invent metrics
or hard-code unlock totals. Keep achievements untouched.

## UI-02 — Level completion screen

Create src/results.cpp and src/results/. Give the successful EndLevelLayer a
MiSide presentation: completion result, actual attempts/jumps and collected coins
where available, plus the original replay/menu actions. Preserve mod-added controls
and their callbacks. Scope is successful completion, not a global PlayLayer/death
overlay. Reuse the existing Win sound without triggering it twice. Leave a clear
layout area for a later Mita reaction; do not add a temporary second character.

## UI-03 — In-game appearance settings

Create src/settings.cpp and src/settings/ for a compact settings page using existing
options: language, particles/effects, transitions and audio controls actually exposed
by the current mod. Group controls, show numeric values, and support reset through
existing mechanisms. No duplicate settings storage. New mod.json keys or missing
Theme capabilities must be noted explicitly for integration. 3D quality, expression
weights and head tracking settings belong to the later 3D integration.

## UI-04 — Official level browser polish

Inspect what is already finished first. Add only missing usability: remember the
selected level/scroll position, keyboard navigation and correct focus on returning,
readable long names, meaningful locked states and actual coin/progress indicators.
The preview renderer is owned by 3D; don't change LevelPreview.*. Don't rebuild the
diskette layout if it already works. Keep original unlock conditions and callbacks.

## UI-05 — Pause usability and compatibility

Improve focus and normal/hover/disabled state restoration after confirmation dialogs,
volume slider interaction and the layout of Additional actions. No change to blur or
the shared OpenGL state. Don't indiscriminately reparent third-party nodes: retain
original actions and an original-UI route for incompatible controls. Change ambient
only for a concrete issue, preserving level music timing and existing Stress logic.

## UI-06 — Small dialogs and layout

Make our confirmations, empty/error states and back navigation consistent. Handle
RU/EN long strings and narrow/wide windows in owned screens, preserving spacing and
input blocking while popups are open. Use existing controls; no global reskin of all
Geode popups or other mods' windows. Existing sparks and Mita hit testing belong to 3D.

## UI-07 — Documentation and credits

Update README/README.ru to describe what currently exists instead of the old
MenuLayer-only scaffold. Add a compact credits/help screen if useful, using actual
attribution from LICENSES.md; don't change asset licenses. Note undocumented controls
and integration needs in docs/ui/. Keep build/install instructions consistent with
docs/PARALLEL_WORK.md during development.

## Later, outside this batch

Icon kit / wardrobe replacing GD's crowded garage, creator hub, user level search,
profiles and editor need a separate compatibility design. Don't replace them wholesale
while the core screens and 3D are being integrated. Plan them in docs/ui/ rather than
starting broad hooks. Mita outfits/expressions and reactions are the 3D lane.

For each finished task: concise changed-files summary, actual checks, remaining
runtime questions and shared changes in docs/ui/INTEGRATION.md. Do not claim in-game
verification without running the relevant combined build.
