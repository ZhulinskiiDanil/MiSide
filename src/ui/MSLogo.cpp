#include "MSLogo.hpp"

#include "../Theme.hpp"
#include "MSStyle.hpp"

using namespace geode::prelude;

namespace
{
  // The text logo's words
  constexpr char const* WORDMARK = "Geometry Dash";
  // The letters' alpha above this is where the gradient shows
  constexpr float LETTER_ALPHA = 0.1f;
  // Where the sparkles sit on the words: shares of their width, and of their line's height from the bottom
  constexpr float SPARKLES[][2] = {{0.06f, 0.78f}, {0.33f, 0.86f}, {0.58f, 0.8f}, {0.9f, 0.84f}, {0.2f, 0.7f}};
  // A sparkle's arms: long ones and short ones, as a share of its size
  constexpr float SPARKLE_THIN = 0.18f;
  constexpr float SPARKLE_SHORT = 0.55f;

  CCNode* letters(ccColor3B color, CCPoint offset)
  {
    auto label = ms::createLabel(WORDMARK, Theme::get().titleFont);
    if (!label)
      return nullptr;
    label->setAnchorPoint({0, 0});
    label->setPosition(offset);
    ms::setColor(label, color);
    return label;
  }

  // A four-armed sparkle
  void sparkle(CCDrawNode* draw, CCPoint at, float size, ccColor4F color)
  {
    auto thin = size * SPARKLE_THIN;
    auto shortArm = size * SPARKLE_SHORT;
    CCPoint vertical[] = {{at.x, at.y + size}, {at.x + thin, at.y}, {at.x, at.y - size}, {at.x - thin, at.y}};
    CCPoint horizontal[] = {{at.x - shortArm, at.y}, {at.x, at.y + thin}, {at.x + shortArm, at.y}, {at.x, at.y - thin}};
    draw->drawPolygon(vertical, 4, color, 0, color);
    draw->drawPolygon(horizontal, 4, color, 0, color);
  }
} // namespace

MSLogo* MSLogo::create()
{
  auto logo = new MSLogo();
  if (logo->init())
  {
    logo->autorelease();
    return logo;
  }
  delete logo;
  return nullptr;
}

CCNode* MSLogo::createWordmark()
{
  auto const& theme = Theme::get();
  auto top = letters(theme.logoGradientBottom, {0, 0});
  if (!top)
    return nullptr;
  auto size = top->getContentSize();
  auto node = CCNode::create();
  auto depth = std::max(static_cast<int>(theme.logoDepthSteps), 0);

  // Under the letters: a soft dark shadow, then darker copies stepping down and right, for depth
  if (auto shadow = letters(theme.logoShadowColor, {theme.logoShadowOffset + depth * theme.logoDepthOffset,
                                                    -theme.logoShadowOffset - depth * theme.logoDepthOffset}))
  {
    ms::setOpacity(shadow, theme.shadowOpacity);
    node->addChild(shadow);
  }
  for (auto step = depth; step >= 1; step--)
  {
    if (auto copy = letters(theme.logoDepthColor, {step * theme.logoDepthOffset, -step * theme.logoDepthOffset}))
      node->addChild(copy);
  }

  // The letters: a gradient from light at the top to deep at the bottom, cut to their shape. Without a stencil
  // buffer, the deep color alone
  if (ms::canClip())
  {
    auto stencil = letters(theme.logoGradientBottom, {0, 0});
    auto clip = CCClippingNode::create(stencil);
    clip->setAlphaThreshold(LETTER_ALPHA);
    auto gradient = CCLayerGradient::create(Theme::withAlpha(theme.logoGradientTop, 255),
                                            Theme::withAlpha(theme.logoGradientBottom, 255));
    gradient->setContentSize(size);
    clip->addChild(gradient);
    node->addChild(clip);
  }
  else
    node->addChild(top);

  // A few sparkles on the letters' tops
  auto count = std::clamp(static_cast<int>(theme.logoSparkles), 0, static_cast<int>(std::size(SPARKLES)));
  if (count > 0 && theme.logoSparkleSize > 0)
  {
    auto draw = CCDrawNode::create();
    auto color = ccc4FFromccc3B(theme.logoSparkleColor);
    for (int i = 0; i < count; i++)
      sparkle(draw, {size.width * SPARKLES[i][0], size.height * SPARKLES[i][1]}, theme.logoSparkleSize / 2, color);
    node->addChild(draw);
  }

  node->setContentSize({size.width + depth * theme.logoDepthOffset + theme.logoShadowOffset, size.height});
  return node;
}

CCNode* MSLogo::createGrid(CCSize area)
{
  auto const& theme = Theme::get();
  if (theme.logoGridOpacity == 0 || theme.logoGridStep <= 0)
    return nullptr;
  auto draw = CCDrawNode::create();
  auto color = ccc4FFromccc3B(theme.logoGridColor);
  color.a = theme.logoGridOpacity / 255.f;
  // Hairlines, a step apart
  auto line = 0.25f;
  for (auto x = 0.f; x <= area.width; x += theme.logoGridStep)
    draw->drawSegment({x, 0}, {x, area.height}, line, color);
  for (auto y = 0.f; y <= area.height; y += theme.logoGridStep)
    draw->drawSegment({0, y}, {area.width, y}, line, color);
  return draw;
}

bool MSLogo::init()
{
  if (!CCNode::init())
    return false;
  auto const& theme = Theme::get();
  CCSize area{theme.logoWidth, theme.logoTop - theme.logoBottom};
  this->setContentSize(area);
  this->setAnchorPoint({0, 0});

  CCNode* art = nullptr;
  if (ms::resourceExists(theme.logoSprite))
    art = CCSprite::create(theme.logoSprite.c_str());
  auto stand = !art;
  if (!art)
    art = this->createWordmark();
  if (!art)
    return false;

  // Fitted into the area, kept to its proportions, at its bottom left corner
  auto size = art->getContentSize();
  if (size.width > 0 && size.height > 0)
    art->setScale(std::min(area.width / size.width, area.height / size.height));
  art->setAnchorPoint({0, 0});
  art->ignoreAnchorPointForPosition(false);
  art->setPosition({0, (area.height - size.height * art->getScale()) / 2});

  // The grid behind the text logo only: real art brings its own
  if (stand)
  {
    if (auto grid = this->createGrid(area))
      this->addChild(grid);
  }
  this->addChild(art);
  return true;
}
