# Pinned 3D dependencies

- cgltf 1.15, commit `360db1a95480fe102ae9c69b27c5d101167ff5ba`, https://github.com/jkuhlmann/cgltf. MIT license in cgltf/LICENSE. Only cgltf.h is used, unmodified.
- GLM 1.0.1, commit `0af55ccecd98d4e5a8d1fad7de25ba429d60e863`, https://github.com/g-truc/glm. License in glm/copying.txt. Header directory copied unmodified.

- stb_image 2.30, commit `2c980bb59875b0d32144a71867fbdebb2f77cd20`, https://github.com/nothings/stb. MIT or public domain (the licence text is at the end of the file). Only stb_image.h is used, unmodified (SHA-256 `594c2fe35d49488b4382dbfaec8f98366defca819d916ac95becf3e75f4200b3`): Mita's atlases are decoded with it where Windows' WIC isn't there (Android).

GLEW is the existing Geode/GD dependency; the mod does not bundle another GL loader.
