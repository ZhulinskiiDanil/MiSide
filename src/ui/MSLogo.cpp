#include "MSLogo.hpp"

#include "../Theme.hpp"
#include "MSStyle.hpp"

using namespace geode::prelude;

namespace
{
  // The text logo's words
  constexpr char const* WORDMARK = "Geometry Dash";
  // The decorations' shape, in shares of their size: the space around them, and where they sit on the text
  constexpr float DECOR_GAP = 0.35f;
  constexpr float DECOR_BASELINE = 0.22f;
  constexpr float SPIKE_WIDTH = 0.8f;
  constexpr float SPIKE_HEIGHT = 0.9f;
  constexpr float DECOR_OUTLINE = 0.5f;

  CCNode* copyOf(ccColor3B color, CCPoint offset)
  {
    auto label = ms::createLabel(WORDMARK);
    if (!label)
      return nullptr;
    label->setAnchorPoint({0, 0});
    label->setPosition(offset);
    ms::setColor(label, color);
    return label;
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
  auto main = copyOf(theme.logoColor, {0, 0});
  if (!main)
    return nullptr;
  auto size = main->getContentSize();
  auto node = CCNode::create();

  // Under it: the dark shadow, then the light copy a little higher, so only its top edge shows over the shadow
  if (auto shadow = copyOf(theme.logoShadowColor, {theme.logoShadowOffset, -theme.logoShadowOffset}))
    node->addChild(shadow);
  if (auto highlight = copyOf(theme.logoHighlightColor, {0, theme.logoHighlightOffset}))
    node->addChild(highlight);
  node->addChild(main);

  // After the words, sitting on their baseline: a GD cube turned on its corner, and a spike
  auto height = size.height * theme.logoDecorSize;
  auto gap = height * DECOR_GAP;
  auto base = size.height * DECOR_BASELINE;
  auto outline = theme.logoShadowOffset * DECOR_OUTLINE;
  auto decor = CCDrawNode::create();
  auto fill = ccc4FFromccc3B(theme.logoDecorColor);
  auto dark = ccc4FFromccc3B(theme.logoShadowColor);
  auto x = size.width + gap;
  CCPoint cube[] = {{x + height / 2, base}, {x + height, base + height / 2}, {x + height / 2, base + height},
                    {x, base + height / 2}};
  decor->drawPolygon(cube, 4, fill, outline, dark);
  x += height + gap;
  CCPoint spike[] = {{x, base}, {x + height * SPIKE_WIDTH, base},
                     {x + height * SPIKE_WIDTH / 2, base + height * SPIKE_HEIGHT}};
  decor->drawPolygon(spike, 3, fill, outline, dark);
  node->addChild(decor);

  node->setContentSize({x + height * SPIKE_WIDTH + theme.logoShadowOffset, size.height});
  return node;
}

bool MSLogo::init()
{
  if (!CCNode::init())
    return false;
  auto const& theme = Theme::get();
  CCSize area{theme.rowWidth, theme.logoTop - theme.logoBottom};
  this->setContentSize(area);
  this->setAnchorPoint({0, 0});

  CCNode* art = nullptr;
  if (ms::resourceExists(theme.logoSprite))
    art = CCSprite::create(theme.logoSprite.c_str());
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
  this->addChild(art);
  return true;
}
