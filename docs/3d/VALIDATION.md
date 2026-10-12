# Windows 3D validation

The standalone test uses the same Model.cpp and Renderer.cpp as the mod, with a hidden
Win32 OpenGL context. It does not load GD, install a mod or emulate cocos UI.

## Reproduce

Run `tools/mita3d/build-preview.ps1` with the existing Geode SDK and Visual C++ tools.
Run `ctest --test-dir build-3d-preview --output-on-failure`.
Put the existing GD directory on this process's PATH so the preview finds GD's glew32.dll:

```powershell
$env:PATH = 'D:\SteamLibrary\steamapps\common\Geometry Dash;' + $env:PATH
./build-3d-preview/mita3d-preview.exe ./resources/mita3d/mita.glb ./build-3d-preview/smile.rgba 0 0
./build-3d-preview/mita3d-preview.exe ./resources/mita3d/mita.glb ./build-3d-preview/exit.rgba 1 0
./build-3d-preview/mita3d-preview.exe ./resources/mita3d/mita.glb ./build-3d-preview/blink.rgba 0 4.39
```

RGBA output is 960x1080, bottom row first, premultiplied alpha. Expression argument is
exit weight 0..1; final argument is elapsed seconds. Files are development outputs only.

## Results on 2026-10-12

- Model test passed: eight meshes/four textures, 21/6 change face vertices, switching back
  exactly restores the pose, direct and stepped time produce the same geometry, no
  non-finite/out-of-bounds vertices across loop boundaries and long elapsed time.
- Preview: NVIDIA RTX 4060, OpenGL 4.6.0, driver 596.49. Visible smile, exit expression and
  closed eyelids inspected against the Blender morph audit. Alpha outside the figure is zero.
- Preview checks framebuffer rendering, resize, silhouette hit testing and restoration of
  deliberately altered GL state. GPU resource release is checked by reopening a renderer.
- Approximate wall-clock render cost at 960x1080: 0.6–1.5 ms/frame including CPU skinning
  and GPU completion, with variation from concurrent compilation. Not a GD FPS measurement.
  Final standalone run: 0.627 ms/frame, initial load 146.5 ms.
- The isolated Windows mod compiles and packages with Geode 5.10.1 / GD 2.2081,
  `GEODE_DONT_INSTALL_MODS=ON`. Build outputs are in build-3d, not the game directory.

The mod build used the existing bindings checkout at
`D:/Dev/MiSide/build/_deps/bindings-src`, commit f8aec5d1f3df76d840dfd2609f02cef30dc6efdf,
and existing dependency source directories read-only. No shared SDK update was performed.

## Still requires the combined game build

- Model registration and exit events are now connected in the integration branch; see ../INTEGRATION_STATUS.md.
- In-game sprite orientation, scale, fade and pink outline at each GD texture quality.
- Menu/achievements transitions, real mouse hit testing, exit hover/confirmation overlap.
- Fullscreen/context recreation and interaction with other mods' framebuffer effects.
- Actual GD frame time and menu-opening latency. First load is synchronous; CPU skinning
  is intentional for this roughly 20k-vertex asset, avoiding a large GPU bone palette.

No in-game verification or cross-platform support is claimed by these tests.

## Right-hand swat revision (2026-10-12)

Matched the supplied MiSide video at 0:03–0:07: arms down, raise the anatomical
right hand, sweep past the face, return outward, lower the arm. The palm frame is
derived from this asset's knuckles. Forearm twist is blended between complete
poses to avoid the +/-pi branch jump during the lift and return.

- The existing model test passes without relaxing its continuity threshold.
- A separate moving-idle scan covered 12 start times with 840 samples each;
  maximum vertex step was 0.000132, below the existing 0.00016 threshold.
- Inspected the exact runtime bone poses in Blender through the official MCP
  server, including front and side views. The review action has 85 frames at 30 fps.
  `mita3d-export-pose <mita.glb> <directory>` exports these poses for inspection.
- Runtime OpenGL preview passed state restoration, hit testing, resize and resource
  release; measured render cost was approximately 0.8 ms/frame on this machine.
- Built on main in `build`; installed the package into GD as explicitly requested.
  Gameplay/input verification in the running game remains a manual check.
