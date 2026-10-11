#pragma once

#include <Geode/Geode.hpp>

// The official levels' pictures: a real frame of each, saved the first time it's played (from its start, without the
// level's own interface), and read back for the level list. Nothing is drawn or saved for other levels
namespace preview
{
  // The level's picture as a texture (cached while the list is open), or null when it hasn't been played yet
  cocos2d::CCTexture2D* texture(int levelID);
  // Drops the cached textures (the list closed)
  void forget();
} // namespace preview
