#pragma once

#include <Geode/Geode.hpp>

#include <string>
#include <utility>
#include <vector>

// What GD's level-complete layer shows, read from it after its own setup (and Geode's node IDs): the numbers as GD
// wrote them in its own labels, the coins it laid out, its end text and message, and its buttons
namespace ms::results
{
  struct Summary
  {
    bool practice = false;
    std::string level;
    // GD's random end text ("Not bad!") and its completion message, when it has them
    std::string endText;
    std::string message;
    // Name and value, from GD's labels (attempts, jumps, time, points), in the menu's language
    std::vector<std::pair<std::string, std::string>> rows;
    // The coins this level has, and the ones collected (as GD's layer shows them)
    int coins = 0;
    int collected = 0;
  };

  Summary read(EndLevelLayer* layer, bool english);
} // namespace ms::results
