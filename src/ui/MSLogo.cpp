#include "MSLogo.hpp"

#include "../Theme.hpp"
#include "MSStyle.hpp"

using namespace geode::prelude;

namespace
{
  // A pixel more opaque than this is part of the drawing; fainter ones are the soft edge's last traces
  constexpr unsigned char DRAWN_ALPHA = 8;
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

bool MSLogo::measure(std::string const& path)
{
  // The same file the sprite shows (the -hd or -uhd one for the texture quality), read once on the main thread
  CCImage image;
  if (!image.initWithImageFile(path.c_str()) || !image.getData() || image.getWidth() == 0 || image.getHeight() == 0)
    return false;
  m_pixels = CCSize(image.getWidth(), image.getHeight());
  if (!image.hasAlpha())
  {
    m_drawn = CCRect(0, 0, m_pixels.width, m_pixels.height);
    return true;
  }
  int left = image.getWidth(), right = -1, top = image.getHeight(), bottom = -1;
  auto data = image.getData();
  for (int y = 0; y < image.getHeight(); y++)
  {
    for (int x = 0; x < image.getWidth(); x++)
    {
      if (data[(static_cast<size_t>(y) * image.getWidth() + x) * 4 + 3] <= DRAWN_ALPHA)
        continue;
      left = std::min(left, x);
      right = std::max(right, x);
      top = std::min(top, y);
      bottom = std::max(bottom, y);
    }
  }
  if (right < left)
    return false;
  m_drawn = CCRect(left, top, right - left + 1, bottom - top + 1);
  return true;
}

bool MSLogo::init()
{
  if (!CCNode::init())
    return false;
  auto const& theme = Theme::get();
  if (!ms::resourceExists(theme.logoSprite))
  {
    log::warn("Logo {} not found, the menu has no logo", theme.logoSprite);
    return false;
  }
  m_sprite = CCSprite::create(theme.logoSprite.c_str());
  std::string path = CCFileUtils::get()->fullPathForFilename(theme.logoSprite.c_str(), false);
  if (!m_sprite || !this->measure(path))
  {
    log::warn("Logo {} can't be read, the menu has no logo", theme.logoSprite);
    return false;
  }
  m_sprite->setAnchorPoint({0, 0});
  this->setAnchorPoint({0, 0});
  this->addChild(m_sprite);
  this->fit(1);
  return true;
}

void MSLogo::fit(float parentScale)
{
  auto const& theme = Theme::get();
  auto content = m_sprite->getContentSize();
  if (content.width <= 0 || m_pixels.width <= 0 || m_drawn.size.width <= 0 || parentScale <= 0)
    return;
  // The image's pixels to its points at this texture quality: the -hd/-uhd file and the sprite's size agree
  auto points = content.width / m_pixels.width;
  CCSize drawn{m_drawn.size.width * points, m_drawn.size.height * points};
  // Proportions kept: one scale both ways
  auto scale = theme.logoWidth / (drawn.width * parentScale);
  m_sprite->setScale(scale);
  // The drawn part's bottom left corner at the node's origin (the image's rows count from the top)
  auto bottomMargin = (m_pixels.height - (m_drawn.origin.y + m_drawn.size.height)) * points;
  m_sprite->setPosition({-m_drawn.origin.x * points * scale, -bottomMargin * scale});
  this->setContentSize({drawn.width * scale, drawn.height * scale});
}
