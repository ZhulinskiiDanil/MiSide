#include "MSBackground.hpp"

#include "../Theme.hpp"
#include "MSStyle.hpp"

#include <cmath>

using namespace geode::prelude;

MSBackground* MSBackground::create(float strength)
{
  auto background = new MSBackground();
  if (background->init(strength))
  {
    background->autorelease();
    return background;
  }
  delete background;
  return nullptr;
}

bool MSBackground::init(float strength)
{
  if (!CCNode::init())
    return false;
  auto const& theme = Theme::get();
  auto winSize = CCDirector::get()->getWinSize();
  m_strength = std::max(strength, 0.f);
  this->setContentSize(winSize);

  for (size_t i = 0; i < theme.layers.size(); i++)
  {
    auto const& spec = theme.layers[i];
    // A margin on every side, so the edges never show when the layer moves
    auto margin = theme.parallaxRange * m_strength * std::abs(spec.depth);
    CCSize cover{winSize.width + margin * 2, winSize.height + margin * 2};

    CCNode* node = nullptr;
    if (ms::resourceExists(spec.sprite))
    {
      if (auto sprite = CCSprite::create(spec.sprite.c_str()))
      {
        auto size = sprite->getContentSize();
        if (size.width > 0 && size.height > 0)
          sprite->setScale(std::max(cover.width / size.width, cover.height / size.height));
        // Black adds nothing: only the light shows
        if (spec.additive)
          sprite->setBlendFunc({GL_ONE, GL_ONE});
        node = sprite;
      }
    }
    if (!node)
    {
      log::warn("Background layer {} not found{}", spec.sprite, i == 0 ? ", the theme's gradient instead" : "");
      if (i > 0)
        continue;
      auto gradient = CCLayerGradient::create(Theme::withAlpha(theme.backdropTop, 255),
                                              Theme::withAlpha(theme.backdropBottom, 255));
      gradient->setContentSize(cover);
      gradient->ignoreAnchorPointForPosition(false);
      node = gradient;
    }
    node->setAnchorPoint({0.5f, 0.5f});
    node->setPosition(winSize / 2);
    this->addChild(node);
    m_layers.push_back({node, spec.depth});
  }

  // Nothing at all: the gradient alone
  if (m_layers.empty())
  {
    auto gradient = CCLayerGradient::create(Theme::withAlpha(theme.backdropTop, 255),
                                            Theme::withAlpha(theme.backdropBottom, 255));
    gradient->setContentSize(winSize);
    this->addChild(gradient);
  }

  if (m_strength > 0 && theme.parallaxRange > 0)
    this->scheduleUpdate();
  return true;
}

void MSBackground::update(float dt)
{
  auto const& theme = Theme::get();
  auto winSize = CCDirector::get()->getWinSize();
  auto center = winSize / 2;
  auto mouse = getMousePos();
  // -1..1 from the screen's center; the layers move the other way, like a camera turning
  CCPoint aim{std::clamp((mouse.x - center.width) / center.width, -1.f, 1.f),
              std::clamp((mouse.y - center.height) / center.height, -1.f, 1.f)};
  auto target = aim * (-theme.parallaxRange * m_strength);
  auto follow = theme.parallaxSmoothing > 0 ? 1.f - std::exp(-dt / theme.parallaxSmoothing) : 1.f;
  m_offset = m_offset + (target - m_offset) * follow;

  for (auto const& layer : m_layers)
    layer.node->setPosition(CCPoint{center.width, center.height} + m_offset * layer.depth);
}
