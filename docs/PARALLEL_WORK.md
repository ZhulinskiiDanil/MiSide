# Parallel work agreement

Prepared from clean main at `4397a24` (Mita click sound with sparks).
Both feature branches start from the same preparation commit; the exact base is
stored in each workspace's ignored `.workspace-zone.json`.

## Ownership

| Owner | Files |
| --- | --- |
| Codex / 3D | New src/mita3d/, resources/mita3d/, third_party/, tools/mita3d/, tests/mita3d/; src/ui/MSMita.cpp/.hpp and MitaPlan.hpp; existing resources/mita/ and mita-closeup/; CMakeLists.txt; attribution in LICENSES.md and ASSETS.md; docs/3d/ |
| Claude / UI | main.cpp and screen hooks; achievements, levels and pause UI; new statistics/results/settings screens; common UI components except MSMita/MitaPlan; Theme.*, Sounds.*, Music.*, loading.cpp; resources/ui/; docs/ui/; README files |
| Integration | mod.json, tests/CMakeLists.txt, collaboration instructions/scripts, and other files outside these areas |

The executable rules are in `tools/work-zones.json`. Rendering-specific existing
files `src/pause/MSPauseBackdrop.*` and `src/levels/LevelPreview.*` are reserved to
3D for this period. UI can change their callers but must retain the current API.
New UI resources live in resources/ui/; registration in mod.json is an integration
change. Prefer existing resources until registration is applied. Keep new 3D
configuration separate from Theme during parallel work; propose its final settings
in docs/3d/INTEGRATION.md. Don't create a second general theme or sound system.

UI owns main.cpp, so Codex must not edit it during parallel work. Integration will
wire exit-hover and confirmation events to the 3D character later. UI continues
to use the current `MSMita::create()` and existing lifecycle. Codex preserves that
public facade. Achievements already create MSMita; no UI rewrite for 3D is needed.
Any new 3D methods are proposed in integration notes before callers depend on them.

## Independent builds

Each workspace can compile independently when needed. Do not use the original
build/ or build-tests/ folders from main, and do not copy CMake caches across roots.
The existing local Geode SDK is read-only/shared; do not update it from either lane.

From the relevant workspace:

```powershell
powershell -NoProfile -File tools/check-zone.ps1
powershell -NoProfile -File tools/build-local.ps1
```

The build helper uses Ninja, clang/clang++, RelWithDebInfo, an in-worktree `build-3d`
or `build-ui` directory and GEODE_DONT_INSTALL_MODS=ON. Prerequisites are the same
as the current Windows project: compiler/Windows SDK environment, CMake, Ninja,
Geode CLI and GEODE_SDK. It does not initialize Visual Studio or download tools.
If not building yet, simply don't run it. A successful compile does not establish
visual or runtime correctness. No builds were needed to create these workspaces.

## Hand-off and integration

1. Finish one coherent task, inspect the diff and run the path guard.
2. Commit the implementation in its feature branch; report commit IDs and checks
   actually performed. Mark untested runtime behavior clearly.
3. Record shared-file edits as concrete file/key/callback requests in the zone's
   INTEGRATION.md. Do not apply another lane's changes by copying files.
4. When both lanes are ready, the integrator creates a fresh integration branch
   from their recorded base, merges both feature branches and resolves shared
   settings/resource registration/events once. Use merge or cherry-pick deliberately,
   not both for the same commits. Keep main unchanged until integration is reviewed.
5. Build that combined branch, then install one combined package for in-game checks.
   Never install UI and 3D variants simultaneously under the same mod ID.

Main remains a usable baseline; existing ignored build/config files there are kept.
Historical local TASK.md and goal.txt are not copied into the feature worktrees.
