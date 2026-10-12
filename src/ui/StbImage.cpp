// stb_image's code, once for the mod: Mita's atlases and her 3D textures are decoded with it where Windows' WIC isn't
#include <Geode/platform/platform.hpp>

#ifndef GEODE_IS_WINDOWS
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include <stb_image.h>
#endif
