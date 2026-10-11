#include "MSLevelCard.hpp"

#include "../Theme.hpp"
#include "../ui/MSStyle.hpp"
#include "LevelPreview.hpp"

using namespace geode::prelude;

namespace
{
  constexpr int FADE_ACTION = 0x4d62;
  // The title font's capitals: 17.5 pt high at scale 1, their middle 2.82 pt over the label's
  constexpr float TITLE_CAP = 17.5f;
  constexpr float CAP_RAISE = 2.82f;
  // The floppy's cut corner, its shadow's offset, the frame's margin and border
  constexpr float CUT = 30;
  constexpr float SHADOW = 5;
  constexpr float MARGIN = 10;
  constexpr float BORDER = 4;
  // The 9-slice's corners are 5.3 pt: thin pieces are made bigger and scaled down
  constexpr float SMALL_PANEL = 2;

  ccColor4F color4(ccColor3B color, float alpha = 1)
  {
    return {color.r / 255.f, color.g / 255.f, color.b / 255.f, alpha};
  }

  bool english() { return Mod::get()->getSettingValue<std::string>("menu-language") == "EN"; }

  std::string difficultyName(GJGameLevel* level)
  {
    auto en = english();
    if (level->m_demon.value() > 0)
      return en ? "DEMON" : "ДЕМОН";
    switch (level->m_difficulty)
    {
    case GJDifficulty::Auto:
      return en ? "AUTO" : "АВТО";
    case GJDifficulty::Easy:
      return en ? "EASY" : "ЛЕГКО";
    case GJDifficulty::Normal:
      return en ? "NORMAL" : "НОРМАЛЬНО";
    case GJDifficulty::Hard:
      return en ? "HARD" : "СЛОЖНО";
    case GJDifficulty::Harder:
      return en ? "HARDER" : "ОЧЕНЬ СЛОЖНО";
    case GJDifficulty::Insane:
      return en ? "INSANE" : "БЕЗУМНО";
    default:
      return en ? "DEMON" : "ДЕМОН";
    }
  }

  // GD's face for the difficulty
  CCSprite* difficultyFace(GJGameLevel* level)
  {
    auto value = level->m_demon.value() > 0 ? 6 : std::clamp(static_cast<int>(level->m_difficulty), 0, 6);
    return CCSprite::createWithSpriteFrameName(fmt::format("difficulty_{:02}_btn_001.png", value).c_str());
  }

  CCNode* panel(ccColor3B color, GLubyte opacity, CCSize size)
  {
    auto node = ms::createPanel(Theme::get().rowSprite, color, opacity);
    node->setAnchorPoint({0, 0.5f});
    node->ignoreAnchorPointForPosition(false);
    node->setContentSize(size * SMALL_PANEL);
    node->setScale(1 / SMALL_PANEL);
    return node;
  }
} // namespace

MSLevelCard* MSLevelCard::create(std::function<void()> play)
{
  auto card = new MSLevelCard();
  if (card->init(std::move(play)))
  {
    card->autorelease();
    return card;
  }
  delete card;
  return nullptr;
}

bool MSLevelCard::init(std::function<void()> play)
{
  if (!CCNode::init())
    return false;
  auto const& theme = Theme::get();
  m_play = std::move(play);
  auto fromTop = CCDirector::get()->getWinSize().height - 320;
  auto left = theme.levelsCardLeft;
  auto right = theme.levelsCardRight;
  auto top = theme.levelsCardTop + fromTop;
  auto bottom = theme.levelsCardBottom + fromTop;

  // The floppy: its shadow down and left, the body with its corner cut, a pink wedge in the cut
  auto shape = CCDrawNode::create();
  auto body = [&](float dx, float dy, ccColor3B color) {
    CCPoint points[] = {{left + dx, top + dy},
                        {right + dx, top + dy},
                        {right + dx, bottom + dy},
                        {left + CUT + dx, bottom + dy},
                        {left + dx, bottom + CUT + dy}};
    shape->drawPolygon(points, 5, color4(color), 0, color4(color));
  };
  body(-SHADOW, -SHADOW, theme.levelsCardShadowColor);
  body(0, 0, theme.levelsCardColor);
  // The picture's window: a pink frame, then a 16:9 opening as wide as the frame leaves
  auto frameLeft = left + MARGIN;
  auto frameRight = right - MARGIN;
  auto frameTop = top - MARGIN;
  auto inner = frameRight - frameLeft - BORDER * 2;
  auto innerHeight = inner * 9 / 16;
  auto frameBottom = frameTop - innerHeight - BORDER * 2;
  CCPoint frame[] = {{frameLeft, frameTop}, {frameRight, frameTop}, {frameRight, frameBottom}, {frameLeft, frameBottom}};
  shape->drawPolygon(frame, 4, color4(theme.levelsCardFrameColor), 0, color4(theme.levelsCardFrameColor));
  m_window = {frameLeft + BORDER, frameBottom + BORDER, inner, innerHeight};
  CCPoint window[] = {{m_window.getMinX(), m_window.getMaxY()},
                      {m_window.getMaxX(), m_window.getMaxY()},
                      {m_window.getMaxX(), m_window.getMinY()},
                      {m_window.getMinX(), m_window.getMinY()}};
  shape->drawPolygon(window, 4, color4(theme.levelsPreviewEmptyColor), 0, color4(theme.levelsPreviewEmptyColor));
  // The label under it, white, a pink wedge where the corner is cut, the shutter's dark hole on the right
  auto labelTop = frameBottom - 6;
  auto labelBottom = bottom + 8;
  CCPoint label[] = {{frameLeft, labelTop}, {frameRight, labelTop}, {frameRight, labelBottom}, {frameLeft + CUT - 10, labelBottom},
                     {frameLeft, labelBottom + CUT - 10}};
  shape->drawPolygon(label, 5, color4(theme.levelsCardLabelColor), 0, color4(theme.levelsCardLabelColor));
  CCPoint wedge[] = {{left, bottom + CUT}, {left + CUT, bottom}, {left + CUT, bottom + CUT}};
  shape->drawPolygon(wedge, 3, color4(theme.levelsCardFrameColor), 0, color4(theme.levelsCardFrameColor));
  shape->drawDot({frameRight - 10, labelBottom + 10}, 5, color4(theme.levelsCardHoleColor));
  this->addChild(shape);

  m_pictures = CCNode::create();
  this->addChild(m_pictures, 1);
  m_info = CCNodeRGBA::create();
  static_cast<CCNodeRGBA*>(m_info)->setCascadeOpacityEnabled(true);
  this->addChild(m_info, 2);

  // PLAY on the label's bottom left: the frame's pink, white text
  m_menu = CCMenu::create();
  m_menu->setPosition({0, 0});
  m_menu->setContentSize({0, 0});
  auto style = MSMenuItem::Style::pause();
  style.plate = theme.levelsCardAccentColor;
  style.plateOpacity = 255;
  style.plateLit = theme.levelsCardFrameColor;
  style.plateLitOpacity = 255;
  style.platePressed = theme.levelsCardShadowColor;
  style.platePressedOpacity = 255;
  style.text = theme.levelsPlayTextColor;
  style.textLit = theme.levelsPlayTextColor;
  style.textPressed = theme.levelsPlayTextColor;
  m_playButton = MSMenuItem::create(english() ? "PLAY" : "ИГРАТЬ", nullptr,
                                    [this](MSMenuItem*) {
                                      ms::playSound(Theme::get().clickSound);
                                      if (m_play)
                                        m_play();
                                    },
                                    100, style);
  if (m_playButton)
  {
    // Clear of the pink wedge in the cut corner
    m_playButton->setPosition({left + CUT + 6, labelBottom + 4 + style.height / 2});
    m_menu->addChild(m_playButton);
  }
  this->addChild(m_menu, 3);
  return true;
}

CCNode* MSLevelCard::text(std::string const& value, float scale, ccColor3B color, float x, float capTop)
{
  auto label = ms::createText(value, scale, color, Theme::get().titleFont, ms::Shadow{});
  if (!label.node)
    return nullptr;
  label.node->setPosition({x, capTop - (TITLE_CAP / 2 + CAP_RAISE) * scale});
  m_info->addChild(label.node);
  return label.node;
}

void MSLevelCard::show(GJGameLevel* level)
{
  if (!level)
    return;
  auto id = level->m_levelID.value();
  if (id == m_levelID)
    return;
  m_levelID = id;
  this->showPicture(level);
  this->showInfo(level);
}

void MSLevelCard::showPicture(GJGameLevel* level)
{
  auto const& theme = Theme::get();
  auto time = std::max(theme.levelsPreviewFade, 0.f);
  // The last picture fades out and goes
  if (m_picture)
  {
    m_picture->stopActionByTag(FADE_ACTION);
    auto out = CCSequence::createWithTwoActions(CCFadeTo::create(time, 0), CCRemoveSelf::create());
    m_picture->runAction(out);
    m_picture = nullptr;
  }
  auto holder = CCNodeRGBA::create();
  holder->setCascadeOpacityEnabled(true);
  auto center = CCPoint{m_window.getMidX(), m_window.getMidY()};
  if (auto texture = preview::texture(level->m_levelID.value()))
  {
    // The frame as it was, its proportions kept: as big as fits the window
    auto sprite = CCSprite::createWithTexture(texture);
    auto size = sprite->getContentSize();
    if (size.width > 0 && size.height > 0)
      sprite->setScale(std::min(m_window.size.width / size.width, m_window.size.height / size.height));
    sprite->setPosition(center);
    holder->addChild(sprite);
  }
  else
  {
    // Not played yet: its difficulty's face, and where the picture will come from
    if (auto face = difficultyFace(level))
    {
      face->setScale(m_window.size.height * 0.42f / std::max(face->getContentHeight(), 1.f));
      face->setPosition(center + CCPoint{0, m_window.size.height * 0.1f});
      holder->addChild(face);
    }
    auto en = english();
    auto note = ms::createText(en ? "PLAY IT ONCE FOR ITS PICTURE" : "ПРЕВЬЮ ПОЯВИТСЯ ПОСЛЕ ЗАПУСКА",
                               0.28f, theme.levelsTextColor, theme.titleFont, ms::Shadow{});
    if (note.node)
    {
      note.node->setAnchorPoint({0.5f, 0.5f});
      note.node->setPosition(center - CCPoint{0, m_window.size.height * 0.3f});
      holder->addChild(note.node);
    }
  }
  holder->setOpacity(0);
  auto in = CCFadeTo::create(time, 255);
  in->setTag(FADE_ACTION);
  holder->runAction(in);
  m_pictures->addChild(holder);
  m_picture = holder;
}

void MSLevelCard::showInfo(GJGameLevel* level)
{
  auto const& theme = Theme::get();
  auto en = english();
  m_info->removeAllChildren();
  auto x = m_window.getMinX();
  auto right = m_window.getMaxX();
  auto top = m_window.getMinY() - BORDER - 6 - 6;
  auto dark = theme.levelsCardTextColor;
  auto accent = theme.levelsCardAccentColor;

  // The name, smaller when it's long
  std::string name = level->m_levelName;
  std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::toupper(c); });
  if (auto node = this->text(name, 0.46f, dark, x, top); node && node->getContentWidth() > right - x)
    node->setScale((right - x) / node->getContentWidth());

  // Difficulty with its face, stars on the right
  auto rowTop = top - 13;
  if (auto face = difficultyFace(level))
  {
    face->setScale(13 / std::max(face->getContentHeight(), 1.f));
    face->setPosition({x + 6.5f, rowTop - 4.5f});
    m_info->addChild(face);
  }
  this->text(difficultyName(level), 0.32f, accent, x + 15, rowTop - 1.5f);
  auto stars = this->text(fmt::format("{}", level->m_stars.value()), 0.32f, dark, right - 22, rowTop - 1.5f);
  if (auto star = CCSprite::createWithSpriteFrameName("star_small01_001.png"))
  {
    star->setScale(8 / std::max(star->getContentHeight(), 1.f));
    star->setPosition({right - 4, rowTop - 4.5f});
    m_info->addChild(star);
    if (stars)
      stars->setPositionX(right - 10 - stars->getContentWidth() * stars->getScale());
  }

  // The secret coins: gold when collected
  auto coinsTop = rowTop - 14;
  this->text(en ? "COINS" : "МОНЕТЫ", 0.28f, dark, x, coinsTop);
  auto stats = GameStatsManager::get();
  for (int i = 0; i < std::max(level->m_coins, 0) && i < 3; i++)
  {
    auto key = level->getCoinKey(i + 1);
    auto have = key && stats->hasSecretCoin(key);
    if (auto coin = CCSprite::createWithSpriteFrameName(have ? "secretCoinUI_001.png" : "secretCoinUI2_001.png"))
    {
      coin->setScale(9 / std::max(coin->getContentHeight(), 1.f));
      coin->setPosition({x + 44 + i * 11.f, coinsTop - 2.5f});
      m_info->addChild(coin);
    }
  }

  // Normal and practice progress: a label, a bar, the number
  auto bar = [&](std::string const& label, int percent, float barTop) {
    this->text(label, 0.28f, dark, x, barTop);
    auto barLeft = x + 46;
    auto barWidth = right - barLeft - 24;
    auto middle = barTop - 2.5f;
    auto track = panel(theme.levelsCardColor, 255, {barWidth, 5});
    track->setPosition({barLeft, middle});
    m_info->addChild(track);
    auto fill = std::clamp(percent, 0, 100) / 100.f;
    if (fill > 0)
    {
      auto filled = panel(accent, 255, {std::max(barWidth * fill, 5.f), 5});
      filled->setPosition({barLeft, middle});
      m_info->addChild(filled);
    }
    this->text(fmt::format("{}%", percent), 0.28f, dark, right - 20, barTop);
  };
  auto progressTop = coinsTop - 13;
  bar(en ? "NORMAL" : "ОБЫЧНЫЙ", level->m_normalPercent.value(), progressTop);
  bar(en ? "PRACTICE" : "ПРАКТИКА", level->m_practicePercent, progressTop - 11);

  // Locked: what it takes (PLAY still asks GD, which says so too)
  auto needed = level->m_requiredCoins;
  auto have = stats->getStat("8");
  if (needed > 0 && have < needed)
  {
    auto lockText = en ? fmt::format("COINS NEEDED: {} ({})", needed, have)
                       : fmt::format("НУЖНО МОНЕТ: {} ({})", needed, have);
    // Over PLAY
    auto buttonX = m_playButton ? m_playButton->getPositionX() : x;
    auto buttonTop = m_playButton ? m_playButton->getPositionY() + m_playButton->getContentHeight() / 2 : progressTop - 24;
    this->text(lockText, 0.28f, accent, buttonX, buttonTop + 8);
  }
}

void MSLevelCard::setShown(bool shown)
{
  this->setVisible(true);
  auto time = std::max(Theme::get().pauseDuration, 0.f);
  if (m_playButton)
  {
    m_playButton->setEnabled(shown);
    m_playButton->resetLook();
    m_playButton->fadeTo(time, shown ? 255 : 0);
  }
}

bool MSLevelCard::onKey(enumKeyCodes key)
{
  if (key != KEY_Enter && key != KEY_Space && key != CONTROLLER_A)
    return false;
  if (m_playButton)
    m_playButton->activate();
  return true;
}
