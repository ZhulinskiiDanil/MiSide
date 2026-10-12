# Mita_Swat integration

Source: Codex workspace `outputs/mita-animation/mita-fbx-anime-workbench.blend`.
The packaged GLB contains the user-authored Mita_Swat action, frames 0–84 at 30 fps.
Export selected meshes and armature, Actions, forced sampling, optimization disabled, morph animations disabled.

Frame zero is the temporary idle. Rapid face clicks play the action once, blending in over 0.18 seconds and out over 0.25 seconds. Menu gaze yields to the authored animation. Default gaze is forward.

The new rig uses metres and named eye bones. Sparse morph data is unpacked at load time. The old procedural arm motion and vertex-index eye offsets are no longer used.

Validation: CPU pose, expression, independent eye, static idle, animation continuity and return tests pass. OpenGL render, state restoration, hit test, resize and release pass. Generated 85 preview frames and inspected a ten-frame contact sheet.
