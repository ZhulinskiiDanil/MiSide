#pragma once

#include <Geode/Geode.hpp>

#include <string>

namespace ms
{
  // A row of numbers, as the statistics': a translucent plate `stats-row-height` high, the name on the left (made
  // smaller when it doesn't fit beside the value), the value on the right in the title font. Its top left corner is at
  // the origin
  cocos2d::CCNode* createValueRow(std::string const& name, std::string const& value, float width);
} // namespace ms
