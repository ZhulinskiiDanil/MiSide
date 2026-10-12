#pragma once

#include <string>
#include <vector>

// GD's statistics as its own Stats window lists them (StatsLayer::customSetup): the same keys in the same order,
// each with GameStatsManager's number and StatsCell's title; "players destroyed" only when there are any, as GD
// does. Shown here in groups, each group in GD's order
namespace ms::stats
{
  struct Row
  {
    std::string key;
    // GD's own (English) title, without its colon; in Russian the mod's translation of it
    std::string title;
    int value = 0;
  };

  struct Group
  {
    std::string title;
    std::vector<Row> rows;
  };

  std::vector<Group> read(bool english);
  // With the language's grouping of thousands: "1 234 567" or "1,234,567"
  std::string format(int value, bool english);
} // namespace ms::stats
