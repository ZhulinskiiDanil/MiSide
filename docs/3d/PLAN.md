# 3D Mita: Windows first

Owner: Codex. Workspace D:/Dev/MiSide-3d, branch feature/mita-3d.
Implemented in the 3D branch. See VALIDATION.md for evidence and INTEGRATION.md for
the remaining shared-file/UI wiring. The standalone renderer is verified; GD runtime
verification happens after combining the branches.

1. Load the existing GLB with pinned cgltf and GLM. Runtime checks OpenGL 3.0 before
   creating FBO/VAO resources; unsupported GPUs keep the existing 2D fallback.
2. Draw the character with transparent background in an owned color/depth target,
   composite through the current MSMita facade. Restore GL state and reconcile cocos
   state caches. Keep UI positioning/callback expectations and current sparks/sound.
3. Skeleton + morphs: target_21 is the user-selected base smile; target_06 is for exit
   hover and exit confirmation. These indices are zero-based GLB morph targets, matching
   the Blender diagnostic labels. Blink uses target_1; don't shift indices because
   Blender's Basis occupies a separate slot. Don't rename/deform the source in place.
4. Gentle infrequent head motion, gaze toward the left menu and optional cursor tracking;
   bone and morph blending with elapsed-time timing, no rapid clip switching.
5. Measure render/load cost and memory, then integrate UI event hooks in main.cpp and
   shared settings/resources on the combined branch. Existing achievements call the
   same facade; they do not need rebuilding.

Existing source (read-only):
`C:/Users/zhuli/Documents/Codex/2026-10-11/files-pasted-by-the-user-codex/outputs/mita-source/source/mita.glb`

Blender reference scene (read-only):
`C:/Users/zhuli/Documents/Codex/2026-10-11/files-pasted-by-the-user-codex/outputs/mita-menu-closeup/mita-menu-closeup.blend`

Morph audit and previews:
`C:/Users/zhuli/Documents/Codex/2026-10-11/files-pasted-by-the-user-codex/outputs/mita-rig-audit/`

Model attribution recorded in source: Elite Big Speakerman / CC-BY-4.0.
Preserve source metadata and consult LICENSES.md when packaging.
