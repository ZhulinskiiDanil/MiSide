# MiSide development

## Current workflow

Parallel work has been merged. The user requested further work directly in
`D:/Dev/MiSide` on `main`. The ownership zones and zone guard below are historical
for the retained feature worktrees. On main, use `git diff --check` and relevant
build/tests. Use build-main with GEODE_DONT_INSTALL_MODS=ON; do not auto-install.


Read `docs/PARALLEL_WORK.md` and the ignored local `.workspace-zone.json` before editing.
The user approved Windows-first real-time 3D Mita and continued interface development.
Earlier TASK.md / goal.txt / README statements limiting the project to MenuLayer are
historical scope, not restrictions on this approved work. Do not silently rewrite
asset attribution or claim unverified compatibility.

## Workspaces

- `D:/Dev/MiSide`: main / integration. No feature work here during parallel work.
- `D:/Dev/MiSide-3d`: `feature/mita-3d`, Codex owns the character and renderer.
- `D:/Dev/MiSide-ui`: `feature/miside-ui`, Claude owns interface development.

Each worktree has its own files and build directory. They share Git history and
objects. Never reset, clean, switch branches, or copy whole directories over the
other person's worktree. Do not delete existing assets or overwrite game installs.
No push, release, or game installation is included in the preparation task.

Run `powershell -NoProfile -File tools/check-zone.ps1` before handing off a commit.
This checks changed paths, not implementation correctness. It is a guardrail,
not an OS access restriction. Shared changes go in your zone's integration notes.

Use `tools/build-local.ps1` if a build is needed: it sets
`GEODE_DONT_INSTALL_MODS=ON`. Never use a build command that auto-installs into the
shared GD directory. Actual in-game testing uses one installed version at a time
and is coordinated with the user. Do not update the shared Geode SDK or compiler.

Keep changes small and commit only your files. Achievements are already implemented;
do not rebuild them. Reuse existing UI/audio helpers and existing vanilla button
callbacks. Preserve mod-added actions and access to the original interface where
needed. No speculative general-purpose framework or broad refactor for a single screen.

See `docs/UI_BACKLOG.md` for UI tasks and `docs/3d/PLAN.md` for the 3D scope.
