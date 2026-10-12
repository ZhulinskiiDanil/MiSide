# 3D integration handoff

3D_STATUS: READY_FOR_INTEGRATION

Windows renderer and MSMita facade are implemented in `feature/mita-3d`.
Implementation commit: `a1a6ac2003250c096d27d23e95ded97dc9645c0a`.
Validation details: `docs/3d/VALIDATION.md`. No installed game files were changed.

Claude: finish and commit the feature already in progress, then hand off. Do not
start another backlog feature after observing this marker.

## Shared integration changes

1. In `mod.json`, append `resources/mita3d/*` to `resources.files`. This includes
   `mita.glb`, `ATTRIBUTION-MITA3D.txt`, `LICENSE-CGLTF.txt` and `LICENSE-GLM.txt`.
   Geode's flattened resources must contain `mita.glb` at the resource root.
   Keep the old atlas resources for the supported fallback. Without this registration,
   the isolated package still displays 2D; compilation alone does not enable 3D in it.
2. In the UI owner's `src/main.cpp`, retain the created MSMita instance and connect:
   - Exit item hover enter/leave: `mita->setExitHovered(true/false)`.
   - Exit confirmation open/close, including cancel/back: `mita->setExitConfirmation(true/false)`.
   - Clear stale hover when changing the menu page or opening confirmation.
   The two reasons are independent. Either true blends to zero-based morph 6; both false
   blend to morph 21. Transition is smoothed internally. Calls are harmless in 2D fallback.
3. Achievements and other existing callers keep `MSMita::create()` unchanged. No direct
   renderer ownership or new character instance is needed inside those screens.

These changes belong to the combined integration branch, not unilateral edits to the
other agent's worktree. Do not copy entire folders between worktrees or install the
isolated package over the running mod.

## Existing settings and behavior

Theme character visibility, position/scale, outline, intro fade, spark visuals and
click sound remain in use. The existing Mita parallax enable flag controls subtle
cursor gaze in 3D. Atlas FPS/freeze-frame controls only apply to the 2D fallback.
No new shared settings or general theme/audio systems were added.

Runtime checks OpenGL 3.0; the standalone test used OpenGL 4.6 on the user's RTX 4060.
First model load is synchronous. Renderer owns its GL objects and renders up to 60 Hz,
at 540–1440 pixels high with up to 4x MSAA. Fullscreen context loss is handled through
the existing shared-program generation signal; actual GD fullscreen testing is pending.

Before installation, build the combined branch and perform the in-game checks in
VALIDATION.md. The status indicates readiness for integration, not completed GD testing.
