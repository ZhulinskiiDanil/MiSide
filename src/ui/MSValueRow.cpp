#include "MSValueRow.hpp"

#include "../Theme.hpp"
#include "MSStyle.hpp"

using namespace geode::prelude;

namespace
{
  // The fonts' capitals: their middle this far over the label's middle, at scale 1
  constexpr float FONT_RAISE = 1.88f;
  constexpr float TITLE_RAISE = 2.82f;
  constexpr float PADDING = 5;
  constexpr float NAME_GAP = 6;
} // namespace

namespace ms
{
  CCNode* createValueRow(std::string const& name, std::string const& value, float width)
  {
    auto const& theme = Theme::get();
    auto height = theme.statsRowHeight;
    auto middle = -height / 2;
    auto row = CCNode::create();
    row->setContentSize({width, height});

    auto plate = createPanel(theme.rowSprite, theme.statsRowColor, theme.statsRowOpacity);
    plate->setAnchorPoint({0, 1});
    plate->ignoreAnchorPointForPosition(false);
    plate->setContentSize({width, height});
    row->addChild(plate);

    auto number = createText(value, theme.statsValueScale, theme.statsValueColor, theme.titleFont, Shadow{});
    auto valueWidth = 0.f;
    if (number.node)
    {
      valueWidth = number.node->getContentWidth();
      number.node->setPosition({width - PADDING - valueWidth, middle - TITLE_RAISE * theme.statsValueScale});
      row->addChild(number.node, 1);
    }
    auto scale = theme.statsLabelScale;
    auto label = createText(name, scale, theme.statsLabelColor);
    auto room = width - PADDING * 2 - valueWidth - NAME_GAP;
    if (label.node && label.node->getContentWidth() > room && room > 0)
    {
      scale *= room / label.node->getContentWidth();
      label = createText(name, scale, theme.statsLabelColor);
    }
    if (label.node)
    {
      label.node->setPosition({PADDING, middle - FONT_RAISE * scale});
      row->addChild(label.node, 1);
    }
    return row;
  }
} // namespace ms
