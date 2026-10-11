#pragma once

#include <Geode/Geode.hpp>

#include <string>
#include <utility>
#include <vector>

// The long bar on the right of MiSide's pause, for the level: its name, and under it small labelled numbers (where
// the attempt is, the normal and practice bests; a platformer's time), each apart, never mixed into one
class MSPauseInfo : public cocos2d::CCNodeRGBA
{
public:
  // stats: label and value, left to right
  static MSPauseInfo* create(std::string const& name, std::vector<std::pair<std::string, std::string>> const& stats);

protected:
  bool init(std::string const& name, std::vector<std::pair<std::string, std::string>> const& stats);
};
