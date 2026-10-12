#pragma once

#include "MSOptionsData.hpp"

#include <string>

// GD's settings' texts as the mod shows them: in Russian where it knows them (by the game variable, else by GD's
// English text), else GD's own; the rows' texts in capitals, on one line. Another mod's option keeps its own text
namespace ms::options
{
  std::string label(Toggle const& toggle, bool en);
  std::string info(Toggle const& toggle, bool en);
  // A page's name from GD (Gameplay, Visual...)
  std::string category(std::string const& name, bool en);
  std::string keyAction(std::string const& action, bool en);
  std::string keyNames(std::string const& keys, bool en);
  // In capitals: Latin and Cyrillic
  std::string upper(std::string const& text);
} // namespace ms::options
