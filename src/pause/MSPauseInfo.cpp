#include "MSPauseInfo.hpp"

#include "../Theme.hpp"
#include "../ui/MSStyle.hpp"

using namespace geode::prelude;

namespace
{
  // The title font's capitals: their middle is 2.82 pt over the label's at scale 1
  constexpr float CAP_RAISE = 2.82f;
} // namespace

MSPauseInfo* MSPauseInfo::create(std::string const& name, std::vector<std::pair<std::string, std::string>> const& stats)
{
  auto info = new MSPauseInfo();
  if (info->init(name, stats))
  {
    info->autorelease();
    return info;
  }
  delete info;
  return nullptr;
}

bool MSPauseInfo::init(std::string const& name, std::vector<std::pair<std::string, std::string>> const& stats)
{
  if (!CCNodeRGBA::init())
    return false;
  auto const& theme = Theme::get();
  this->setCascadeOpacityEnabled(true);
  auto winSize = CCDirector::get()->getWinSize();
  auto left = theme.pauseBarLeft;
  auto width = std::max(winSize.width - theme.pauseBarRightMargin - left, 40.f);
  auto height = theme.pauseRowHeight;
  auto top = winSize.height - (320 - theme.pauseFirstRowTop);
  this->setAnchorPoint({0, 0});
  this->setPosition({left, top - height});
  this->setContentSize({width, height});

  // A thin light border, then the dark bar
  auto border = ms::createPanel(theme.rowSprite, theme.pauseBarBorderColor, theme.pauseBarBorderOpacity);
  border->setAnchorPoint({0.5f, 0.5f});
  border->ignoreAnchorPointForPosition(false);
  border->setContentSize({width + 1.2f, height + 1.2f});
  border->setPosition({width / 2, height / 2});
  ms::cascadeOpacity(border);
  this->addChild(border);
  auto bar = ms::createPanel(theme.rowSprite, theme.pauseBarColor, theme.pauseBarOpacity);
  bar->setAnchorPoint({0, 0});
  bar->ignoreAnchorPointForPosition(false);
  bar->setContentSize({width, height});
  ms::cascadeOpacity(bar);
  this->addChild(bar);

  // The level's name in the middle, smaller when it's too long for the bar
  if (auto label = ms::createText(name, theme.pauseBarTextScale, theme.pauseBarTextColor, theme.titleFont, ms::Shadow{});
      label.node)
  {
    auto room = width - theme.pauseTextPadding * 4;
    auto scale = label.node->getContentWidth() > room ? room / label.node->getContentWidth() : 1.f;
    label.node->setScale(scale);
    label.node->setAnchorPoint({0.5f, 0.5f});
    label.node->setPosition({width / 2, height / 2 - CAP_RAISE * theme.pauseBarTextScale * scale});
    this->addChild(label.node);
  }

  // The numbers under the bar, from its right end leftwards: a small plate each, the label pink, the value white
  auto x = width;
  auto chipHeight = theme.pauseStatsHeight;
  auto y = -theme.pauseStatsGap - chipHeight / 2;
  for (auto it = stats.rbegin(); it != stats.rend(); ++it)
  {
    auto labelText = ms::createText(it->first + " ", theme.pauseStatsScale, theme.pauseStatsLabelColor, theme.titleFont,
                                    ms::Shadow{});
    auto valueText = ms::createText(it->second, theme.pauseStatsScale, theme.pauseStatsValueColor, theme.titleFont,
                                    ms::Shadow{});
    if (!labelText.node || !valueText.node)
      continue;
    auto padding = theme.pauseTextPadding;
    auto chipWidth = labelText.node->getContentWidth() + valueText.node->getContentWidth() + padding * 2;
    auto chip = ms::createPanel(theme.rowSprite, theme.pauseBarColor, theme.pauseStatsOpacity);
    chip->setAnchorPoint({1, 0.5f});
    chip->ignoreAnchorPointForPosition(false);
    // Its corners as small as it is
    chip->setContentSize(CCSize{chipWidth, chipHeight} * 2);
    chip->setScale(0.5f);
    chip->setPosition({x, y});
    ms::cascadeOpacity(chip);
    this->addChild(chip);
    auto textY = y - CAP_RAISE * theme.pauseStatsScale;
    labelText.node->setAnchorPoint({0, 0.5f});
    labelText.node->setPosition({x - chipWidth + padding, textY});
    this->addChild(labelText.node);
    valueText.node->setAnchorPoint({0, 0.5f});
    valueText.node->setPosition({x - chipWidth + padding + labelText.node->getContentWidth(), textY});
    this->addChild(valueText.node);
    x -= chipWidth + theme.pauseStatsGap;
  }
  return true;
}
