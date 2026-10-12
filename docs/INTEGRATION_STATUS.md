# Combined Windows build

Branch: `integration/mita-3d-ui`, workspace: `D:/Dev/MiSide-integration`.
Merged `feature/mita-3d` at `002d450` and `feature/miside-ui` at `f041ffc`.
The original main and both feature worktrees remain unchanged.

## Connected

- Registered `resources/mita3d/*`; the package contains the model and its attribution/licenses.
- Main menu keeps its MSMita instance and connects exit mouse hover and confirmation separately.
- Busy/covered menus clear hover; opening confirmation clears hover but retains the exit expression.
- Cancelling confirmation returns to smile unless the cursor is again over Exit.
- UI screens retain MSMita::create(); credits mention real-time 3D and the atlas fallback.
- build-local.ps1 accepts the integration workspace and still disables installation.

## Validation

- Windows combined DLL and .geode build succeeded (Geode 5.10.1 / GD 2.2081).
- GEODE_DONT_INSTALL_MODS=ON; no game installation was performed.
- Existing mita-plan, dodge-spring and shine-pass tests: 3/3 passed.
- Packaged GLB SHA-256 matches resources/mita3d/mita.glb.
- Integration diff whitespace check passed.
- Renderer standalone evidence remains in docs/3d/VALIDATION.md.
- Build/configuration/test logs: build-integration/*.log.

The three optional UI-03 effects settings are deferred; current appearance settings work
with the existing registered keys. No unconnected settings were added.

## Pending in-game verification

Install one combined package in coordination with the user, then check the menu,
exit hover/confirmation/cancel, Mita clicks, achievements/statistics/settings/results,
fullscreen changes, pause blur, and third-party mod access. Check RU/EN and window
aspect ratios. Compilation and standalone tests do not establish runtime UI compatibility.

## Return from achievements: rendering fix

User reported a mostly black screen after Main menu -> Achievements -> Back;
moving the Windows game window restored drawing. MSMita::release3D called
ccGLInvalidateStateCache, which also calls kmGLFreeAll in cocos2d-x 2.x and
invalidates the scene projection stacks. Replaced the global reset with
ccGLBindTexture2D(0) before renderer destruction, keeping the cocos texture cache
consistent without touching matrices. Context-loss disposal still abandons stale handles.

Reference: https://github.com/cocos2d/cocos2d-x/blob/cocos2d-x-2.2.3/cocos2dx/shaders/ccGLStateCache.cpp
The rebuilt package still needs the reported navigation sequence checked in GD.

## Expression selection correction

User corrected the chosen expressions: default is target_14, exit hover and
confirmation use basis (no expression morph). Blink target_1 remains independent;
the existing smoothed exit weight fades the smile to zero. Supersedes the earlier
21/6 mapping in the historical hand-off and validation notes.

## User acceptance

On 2026-10-12 the user reported that everything works with the package containing
92ede09 and requested merging to main. This confirms the reported user workflow;
it is not a claim that every compatibility, language, or aspect-ratio case above
was independently exercised. No known blocking issue remains from this hand-off.
Optional effects settings remain deferred.

## Main-menu gaze (cc98f25)

Development continues in D:/Dev/MiSide on main at the user's request.
Hovered menu buttons supply their world-space centers for smoothed eye tracking.
Exit hover retains basis; opening exit confirmation now overrides it with smile
14 and a forward gaze. Cancelling restores menu tracking. This supersedes the
previous basis-during-confirmation behavior.

Build-main succeeded with installation disabled. Model test and standalone GL
checks passed. Menu-direction and forward-gaze previews were visually inspected.
Actual interaction in GD remains for user verification.

## Gaze tuning from reference screenshots

Increased head yaw/pitch toward hovered menu items and vertical targeting range.
Head aiming uses the neutral rig orientation instead of adding onto animated head
rotation; body/neck translation and procedural sway remain animated. Independent
eye rotation is smaller than the head turn. Confirmation retains smile 14 with
forward gaze. Top/bottom/confirmation renders were inspected, and the model and
GL checks passed. Main builds now go to build/ at the user request.
