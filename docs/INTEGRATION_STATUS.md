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
