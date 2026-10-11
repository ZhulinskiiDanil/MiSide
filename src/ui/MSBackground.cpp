#include "MSBackground.hpp"

#include "../Theme.hpp"
#include "MSParticles.hpp"
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
  m_strength = std::max(strength, 0.f);
  this->setContentSize(CCDirector::get()->getWinSize());

  this->addLayers();
  this->addOverlays();

  if (m_strength > 0 && theme.parallaxRange > 0)
    this->scheduleUpdate();
  return true;
}

void MSBackground::addLayers()
{
  auto const& theme = Theme::get();
  auto winSize = CCDirector::get()->getWinSize();

  // One margin for all the layers, so art of the same size is scaled and cropped the same way
  auto deepest = 0.f;
  for (auto const& spec : theme.layers)
    deepest = std::max(deepest, std::abs(spec.depth));
  auto margin = theme.parallaxRange * m_strength * deepest;
  CCSize cover{winSize.width + margin * 2, winSize.height + margin * 2};
  // The art's focus point sits on the screen's: any crop stays inside the art
  auto focusX = std::clamp(theme.backgroundFocusX, 0.f, 1.f);
  auto focusY = std::clamp(theme.backgroundFocusY, 0.f, 1.f);
  CCPoint home{winSize.width * focusX, winSize.height * focusY};

  for (size_t i = 0; i < theme.layers.size(); i++)
  {
    auto const& spec = theme.layers[i];
    CCNode* node = nullptr;
    if (ms::resourceExists(spec.sprite))
    {
      if (auto sprite = CCSprite::create(spec.sprite.c_str()))
      {
        auto size = sprite->getContentSize();
        if (size.width > 0 && size.height > 0)
          sprite->setScale(std::max(cover.width / size.width, cover.height / size.height));
        sprite->setColor(spec.tint.value_or(spec.additive ? ccWHITE : theme.backgroundTint));
        sprite->setOpacity(spec.opacity);
        // Black adds nothing: only the light shows. A premultiplied texture has its color scaled by the
        // opacity already; otherwise the opacity scales it while it's added
        if (spec.additive)
        {
          auto texture = sprite->getTexture();
          auto premultiplied = texture && texture->hasPremultipliedAlpha();
          sprite->setBlendFunc({static_cast<GLenum>(premultiplied ? GL_ONE : GL_SRC_ALPHA), GL_ONE});
        }
        sprite->setAnchorPoint({focusX, focusY});
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
      gradient->setAnchorPoint({focusX, focusY});
      node = gradient;
    }
    node->setPosition(home);
    this->addChild(node);
    m_layers.push_back({node, home, spec.depth});
  }

  // Nothing at all: the gradient alone
  if (m_layers.empty())
  {
    auto gradient = CCLayerGradient::create(Theme::withAlpha(theme.backdropTop, 255),
                                            Theme::withAlpha(theme.backdropBottom, 255));
    gradient->setContentSize(winSize);
    this->addChild(gradient);
  }
}

void MSBackground::addOverlays()
{
  auto const& theme = Theme::get();
  auto winSize = CCDirector::get()->getWinSize();

  // Over the whole scene: darker
  if (theme.darkenOpacity > 0)
  {
    auto darken = CCLayerColor::create(Theme::withAlpha(theme.darkenColor, theme.darkenOpacity));
    darken->setContentSize(winSize);
    this->addChild(darken);
  }

  this->addPattern();

  // A soft crimson light on the right: a color, not a brightness
  if (theme.glowOpacity > 0 && theme.glowWidth > 0 && theme.glowHeight > 0)
  {
    if (ms::resourceExists(theme.glowSprite))
    {
      if (auto glow = ms::createStretched(theme.glowSprite, {theme.glowWidth, theme.glowHeight}, theme.glowColor,
                                          theme.glowOpacity))
      {
        glow->setAnchorPoint({0.5f, 0.5f});
        glow->setPosition({winSize.width * theme.glowX, winSize.height * theme.glowY});
        this->addChild(glow);
      }
    }
    else
      log::warn("Glow sprite {} not found, no glow", theme.glowSprite);
  }

  // Slow pink motes, over the room and under the menu
  if (auto particles = MSParticles::create())
    this->addChild(particles);

  this->addCharacter();

  // The left side, under the menu, darkest at the edge
  if (theme.leftShadeOpacity > 0 && theme.leftShadeWidth > 0)
  {
    auto shade = CCLayerGradient::create(Theme::withAlpha(theme.leftShadeColor, theme.leftShadeOpacity),
                                         Theme::withAlpha(theme.leftShadeColor, 0), {1, 0});
    shade->setContentSize({theme.leftShadeWidth, winSize.height});
    this->addChild(shade);
  }

  // Dark edges, last
  if (theme.vignetteOpacity > 0 && ms::resourceExists(theme.vignetteSprite))
  {
    if (auto vignette = ms::createStretched(theme.vignetteSprite, winSize, theme.vignetteColor, theme.vignetteOpacity))
    {
      vignette->setAnchorPoint({0, 0});
      this->addChild(vignette);
    }
  }
}

void MSBackground::addPattern()
{
  auto const& theme = Theme::get();
  if (theme.patternOpacity == 0 || theme.patternTile <= 0 || !ms::resourceExists(theme.patternSprite))
    return;
  auto sprite = CCSprite::create(theme.patternSprite.c_str());
  if (!sprite)
    return;
  auto texture = sprite->getTexture();
  auto tile = texture->getContentSize();
  // Repeating needs a power-of-two texture; another size would show one tile stretched instead
  auto power = [](unsigned v) { return v && !(v & (v - 1)); };
  if (!power(texture->getPixelsWide()) || !power(texture->getPixelsHigh()) || tile.width <= 0)
  {
    log::warn("Pattern {} isn't a power of two on each side, left out", theme.patternSprite);
    return;
  }
  ccTexParams params{GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT};
  texture->setTexParameters(&params);
  // The texture repeats over the whole screen; the sprite is scaled so one tile is `pattern-tile` points
  auto winSize = CCDirector::get()->getWinSize();
  auto scale = theme.patternTile / tile.width;
  sprite->setTextureRect({0, 0, winSize.width / scale, winSize.height / scale});
  sprite->setScale(scale);
  sprite->setAnchorPoint({0, 0});
  sprite->setColor(theme.patternColor);
  sprite->setOpacity(theme.patternOpacity);
  this->addChild(sprite);
}

void MSBackground::addCharacter()
{
  auto const& theme = Theme::get();
  // Off unless the theme names the art
  if (theme.characterSprite.empty())
    return;
  if (!ms::resourceExists(theme.characterSprite))
  {
    log::warn("Character {} not found, none", theme.characterSprite);
    return;
  }
  auto sprite = CCSprite::create(theme.characterSprite.c_str());
  if (!sprite)
    return;
  auto winSize = CCDirector::get()->getWinSize();
  auto size = sprite->getContentSize();
  if (size.height > 0)
    sprite->setScale(winSize.height * theme.characterHeight / size.height);
  sprite->setOpacity(theme.characterOpacity);
  // Standing on the bottom edge, its head at the top edge
  sprite->setAnchorPoint({0.5f, 0});
  CCPoint home{winSize.width * theme.characterX, 0};
  sprite->setPosition(home);
  this->addChild(sprite);
  m_layers.push_back({sprite, home, theme.characterDepth});
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
    layer.node->setPosition(layer.home + m_offset * layer.depth);
}
