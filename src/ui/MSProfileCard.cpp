#include "MSProfileCard.hpp"

#include "../Theme.hpp"
#include "MSStyle.hpp"

using namespace geode::prelude;

MSProfileCard* MSProfileCard::create(CCMenuItem* profileButton, std::string const& name)
{
  auto card = new MSProfileCard();
  if (card->init(profileButton, name))
  {
    card->autorelease();
    return card;
  }
  delete card;
  return nullptr;
}

CCNode* MSProfileCard::createAvatar()
{
  auto const& theme = Theme::get();
  auto avatar = CCNode::create();
  avatar->setContentSize({theme.avatarSize, theme.avatarSize});
  avatar->setAnchorPoint({0.5f, 0.5f});
  auto center = avatar->getContentSize() / 2;

  if (auto circle = ms::createStretched(theme.avatarSprite, {theme.avatarSize, theme.avatarSize}, theme.avatarColor,
                                        theme.avatarOpacity))
  {
    circle->setAnchorPoint({0.5f, 0.5f});
    circle->setPosition(center);
    avatar->addChild(circle);
  }

  // The player's cube, in their colors
  auto gm = GameManager::get();
  auto frame = gm->getPlayerFrame();
  if (auto player = SimplePlayer::create(frame))
  {
    player->updatePlayerFrame(frame, IconType::Cube);
    player->setColor(gm->colorForIdx(gm->getPlayerColor()));
    player->setSecondColor(gm->colorForIdx(gm->getPlayerColor2()));
    if (gm->getPlayerGlow())
      player->setGlowOutline(gm->colorForIdx(gm->getPlayerGlowColor()));
    else
      player->disableGlowOutline();
    auto icon = player->m_firstLayer ? player->m_firstLayer->getContentSize() : CCSize{};
    auto size = std::max(icon.width, icon.height);
    if (size > 0)
      player->setScale(theme.avatarSize * theme.avatarIcon / size);
    player->setPosition(center);
    avatar->addChild(player);
  }
  return avatar;
}

bool MSProfileCard::init(CCMenuItem* profileButton, std::string const& name)
{
  if (!CCMenu::init() || !profileButton)
    return false;
  auto const& theme = Theme::get();
  this->setPosition({0, 0});
  this->setContentSize({0, 0});

  auto avatar = this->createAvatar();
  auto text = ms::createText(name, theme.nameScale, theme.nameColor);
  auto textSize = text.node ? text.node->getContentSize() : CCSize{};
  auto height = std::max(theme.avatarSize, textSize.height) + theme.cardPadding * 2;
  auto width = theme.cardPadding * 2 + theme.avatarSize + (text.node ? theme.cardGap + textSize.width : 0);

  auto card = CCNode::create();
  card->setContentSize({width, height});
  if (auto panel = ms::createPanel(theme.traySprite, theme.cardColor, theme.cardOpacity))
  {
    panel->setAnchorPoint({0, 0});
    panel->ignoreAnchorPointForPosition(false);
    panel->setContentSize({width, height});
    card->addChild(panel);
  }
  avatar->setPosition({theme.cardPadding + theme.avatarSize / 2, height / 2});
  card->addChild(avatar);
  if (text.node)
  {
    text.node->setPosition({theme.cardPadding + theme.avatarSize + theme.cardGap, height / 2});
    card->addChild(text.node);
  }

  Ref<CCMenuItem> target = profileButton;
  auto button = CCMenuItemExt::createSpriteExtra(card, [target](auto) {
    ms::playSound(Theme::get().clickSound);
    target->activate();
  });
  button->m_scaleMultiplier = theme.cardPressScale;
  button->setID("profile-card-button"_spr);
  button->setPosition({theme.cardLeft + width / 2, theme.cardBottom + height / 2});
  this->addChild(button);
  m_button = button;
  return true;
}

CCRect MSProfileCard::getArea()
{
  if (!m_button)
    return {};
  // Without a press animation's grow
  auto size = m_button->getContentSize();
  auto position = m_button->getPosition();
  CCRect box{position.x - size.width / 2, position.y - size.height / 2, size.width, size.height};
  return CCRectApplyAffineTransform(box, this->nodeToParentTransform());
}
