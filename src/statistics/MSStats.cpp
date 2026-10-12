#include "MSStats.hpp"

#include <Geode/Geode.hpp>

#include <cstdlib>

using namespace geode::prelude;

namespace
{
  struct Key
  {
    char const* key;
    // The mod's Russian for GD's title
    char const* ru;
    int group;
    // Shown only above zero (GD shows "players destroyed" so)
    bool onlyAboveZero = false;
  };

  // StatsLayer::customSetup's keys in its order (GD 2.2081), each in one of the groups below. Their meaning is
  // StatsCell::getTitleFromKey's
  constexpr Key KEYS[] = {
    {"1", "Прыжки", 0},
    {"2", "Попытки", 0},
    {"6", "Звёзды", 1},
    {"13", "Алмазы", 1},
    {"22", "Луны", 1},
    {"3", "Официальные уровни", 2},
    {"4", "Онлайн-уровни", 2},
    {"42", "Безумные уровни", 2},
    {"5", "Демоны", 2},
    {"15", "Ежедневные уровни", 2},
    {"8", "Секретные монеты", 1},
    {"12", "Монеты игроков", 1},
    {"7", "Мап-паки", 2},
    {"40", "Гаунтлеты", 2},
    {"10", "Лайки и дизлайки", 3},
    {"11", "Оценённые уровни", 3},
    {"41", "Награды за списки", 1},
    {"9", "Уничтожено игроков", 0, true},
  };

  constexpr char const* GROUPS_RU[] = {"ИГРА", "СОБРАНО", "ПРОЙДЕНО", "ОЦЕНКИ"};
  constexpr char const* GROUPS_EN[] = {"PLAYING", "COLLECTED", "COMPLETED", "RATINGS"};
} // namespace

namespace ms::stats
{
  std::vector<Group> read(bool english)
  {
    std::vector<Group> groups;
    for (int i = 0; i < 4; i++)
      groups.push_back({english ? GROUPS_EN[i] : GROUPS_RU[i], {}});
    auto manager = GameStatsManager::sharedState();
    if (!manager)
      return {};
    // A cell only to ask it for GD's titles
    Ref<StatsCell> cell = new StatsCell("stats", 300, 30);
    cell->release();
    for (auto const& entry : KEYS)
    {
      auto value = manager->getStat(entry.key);
      if (entry.onlyAboveZero && value <= 0)
        continue;
      std::string title;
      if (english)
      {
        auto gd = cell->getTitleFromKey(entry.key);
        title = gd ? gd : entry.key;
        if (!title.empty() && title.back() == ':')
          title.pop_back();
      }
      else
        title = entry.ru;
      groups[entry.group].rows.push_back({entry.key, title, value});
    }
    // No empty group
    std::erase_if(groups, [](auto const& group) { return group.rows.empty(); });
    return groups;
  }

  std::string format(int value, bool english)
  {
    auto digits = std::to_string(std::abs(value));
    std::string out;
    for (size_t i = 0; i < digits.size(); i++)
    {
      if (i > 0 && (digits.size() - i) % 3 == 0)
        out += english ? ',' : ' ';
      out += digits[i];
    }
    return value < 0 ? "-" + out : out;
  }
} // namespace ms::stats
