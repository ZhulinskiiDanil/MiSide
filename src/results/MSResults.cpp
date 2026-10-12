#include "MSResults.hpp"

using namespace geode::prelude;

namespace
{
  struct Label
  {
    char const* id;
    // GD's own word before the colon, and the mod's Russian for it
    char const* en;
    char const* ru;
  };

  // geode-sdk/NodeIDs, EndLevelLayer.cpp: GD's labels, in the summary container
  constexpr Label LABELS[] = {
    {"attempts-label", "Attempts", "Попытки"},
    {"jumps-label", "Jumps", "Прыжки"},
    {"time-label", "Time", "Время"},
    {"points-label", "Points", "Очки"},
  };

  std::string trim(std::string text)
  {
    auto first = text.find_first_not_of(" \t");
    auto last = text.find_last_not_of(" \t");
    return first == std::string::npos ? std::string() : text.substr(first, last - first + 1);
  }

  // A TextArea's lines, as it laid them out, joined again
  std::string textOf(TextArea* area)
  {
    std::string text;
    if (!area || !area->m_label || !area->m_label->m_lines)
      return text;
    for (auto line : CCArrayExt<CCNode*>(area->m_label->m_lines))
    {
      auto label = typeinfo_cast<CCLabelBMFont*>(line);
      if (!label)
        continue;
      if (!text.empty())
        text += ' ';
      text += label->getString();
    }
    return trim(text);
  }
} // namespace

namespace ms::results
{
  Summary read(EndLevelLayer* layer, bool english)
  {
    Summary summary;
    if (!layer || !layer->m_mainLayer)
      return summary;
    auto main = layer->m_mainLayer;
    summary.practice = main->getChildByID("practice-complete-text") != nullptr;
    if (layer->m_playLayer && layer->m_playLayer->m_level)
      summary.level = std::string(layer->m_playLayer->m_level->m_levelName);

    for (auto const& entry : LABELS)
    {
      auto label = typeinfo_cast<CCLabelBMFont*>(layer->getChildByIDRecursive(entry.id));
      if (!label)
        continue;
      std::string text = label->getString();
      auto colon = text.find(':');
      if (colon == std::string::npos)
        continue;
      summary.rows.emplace_back(english ? entry.en : entry.ru, trim(text.substr(colon + 1)));
    }
    if (auto end = typeinfo_cast<CCLabelBMFont*>(main->getChildByID("end-text")))
      summary.endText = trim(end->getString());
    summary.message = textOf(typeinfo_cast<TextArea*>(main->getChildByID("complete-message")));

    // Every coin of the level has its dark place; the collected ones have their coin over it
    for (int i = 1; main->getChildByID(fmt::format("coin-{}-background", i)); i++)
    {
      summary.coins++;
      if (layer->getChildByIDRecursive(fmt::format("coin-{}-sprite", i)))
        summary.collected++;
    }
    return summary;
  }
} // namespace ms::results
