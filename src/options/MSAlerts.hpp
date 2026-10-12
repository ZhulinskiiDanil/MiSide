#pragma once

#include <Geode/Geode.hpp>

#include <string>

// GD's popups while the settings screens are open: a plain popup (an FLAlertLayer with its title, text and one or two
// buttons) or a character's dialog shows as the mod's screen instead, its buttons doing what GD's do (the popup's
// delegate gets the same answer, the dialog's the same close). A popup with more in it than its texts stays GD's
namespace ms::options
{
  // A popup a GD window made in itself while it wasn't shown: as the mod's screen when it can be, else over everything
  void adoptPopup(cocos2d::CCNode* node, int z);
  bool showAlert(FLAlertLayer* alert);
  void showDialog(DialogLayer* dialog);
  // GD's text without its markup (<cy>, </c>, <d010>...)
  std::string plain(std::string const& text);
} // namespace ms::options
