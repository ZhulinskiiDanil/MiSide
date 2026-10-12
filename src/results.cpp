#include "Theme.hpp"
#include "results/MSResults.hpp"
#include "ui/MSButtonMenu.hpp"
#include "ui/MSMenuItem.hpp"
#include "ui/MSStyle.hpp"
#include "ui/MSValueRow.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/EndLevelLayer.hpp>

using namespace geode::prelude;

namespace
{
  // The fonts' capitals: their height and their middle over the label's middle, at scale 1
  constexpr float TITLE_CAP = 17.5f;
  constexpr float TITLE_RAISE = 2.82f;
  constexpr float FONT_CAP = 11.7f;
  constexpr float FONT_RAISE = 1.88f;
  constexpr int CARD_Z = 20;
  // The card's right edge stays this far left of the screen's middle
  constexpr float NARROW_GAP = 6;

  // GD's own nodes of the layer by Geode's IDs (geode-sdk/NodeIDs, EndLevelLayer.cpp): hidden while the card shows,
  // never removed. The stars, moons, orbs and diamonds GD flies in after it opens stay as they are
  constexpr char const* VANILLA[] = {"background",      "chain-left",         "chain-right",
                                     "level-complete-text", "practice-complete-text", "summary-container",
                                     "end-text",        "complete-message",   "controller-retry-hint",
                                     "controller-exit-hint"};

  struct Button
  {
    char const* id;
    char const* ru;
    char const* en;
  };

  // GD's buttons in the card's order; the ones the level doesn't have are left out. Other buttons in GD's menu (the
  // mods') stay where they are
  constexpr Button BUTTONS[] = {
    {"retry-button", "ЕЩЁ РАЗ", "REPLAY"},
    {"practice-retry-button", "С ЧЕКПОИНТА", "FROM CHECKPOINT"},
    {"edit-button", "РЕДАКТОР", "EDIT"},
    {"leaderboard-button", "ЛИДЕРЫ", "LEADERBOARD"},
    {"exit-button", "В МЕНЮ", "MENU"},
  };

  bool english() { return Mod::get()->getSettingValue<std::string>("menu-language") == "EN"; }

  // A line of text in the card, centred, made smaller when it's wider than the card; null when it can't be made
  CCNode* line(std::string const& text, float scale, ccColor3B color, std::string const& font, float width,
               ms::Shadow const& shadow = {})
  {
    auto made = ms::createText(text, scale, color, font, shadow);
    if (made.node && made.node->getContentWidth() > width && width > 0)
      made = ms::createText(text, scale * width / made.node->getContentWidth(), color, font, shadow);
    return made.node;
  }
} // namespace

// The level-complete layer as a card of MiSide's on the left, in GD's own layer (it drops in as GD's does, GD's hide
// button hides it): LEVEL COMPLETE (or PRACTICE COMPLETE), the level's name, GD's end text and message, GD's own
// numbers (attempts, jumps, time, points) and coins, and the buttons, each pressing GD's own (its hooks, its sounds,
// the edit button only where GD has it). GD's frame, chains and labels hide; the stars, orbs and diamonds it flies in
// and every button other mods add stay. The right of the screen is left clear (for Mita's reaction, later)
class $modify(MSEndLevelLayer, EndLevelLayer)
{
  static void onModify(auto& self)
  {
    // After Geode's node IDs
    if (self.setHookPriorityPost("EndLevelLayer::customSetup", Priority::VeryLate).isErr())
      log::warn("Can't set the EndLevelLayer::customSetup hook's priority");
  }

  void customSetup()
  {
    EndLevelLayer::customSetup();
    if (Mod::get()->getSettingValue<bool>("vanilla-menu") || !Theme::get().resultsEnabled || !m_mainLayer)
      return;
    this->buildCard();
  }

  void buildCard()
  {
    auto const& theme = Theme::get();
    auto en = english();
    auto summary = ms::results::read(this, en);
    auto main = m_mainLayer;
    auto menu = typeinfo_cast<CCMenu*>(main->getChildByID("button-menu"));
    if (!menu || !menu->getChildByID("retry-button") || !menu->getChildByID("exit-button"))
    {
      log::warn("Level complete: GD's buttons not found, its own layer stays");
      return;
    }

    // Narrow windows (4:3): up to the screen's middle, where GD flies its stars and orbs in
    auto winSize = CCDirector::get()->getWinSize();
    auto width = std::min(theme.resultsWidth, winSize.width / 2 - theme.resultsLeft - NARROW_GAP);
    auto inner = width - theme.resultsPadding * 2;
    auto card = CCNode::create();
    card->setID("results-card"_spr);
    auto y = -theme.resultsPadding;
    auto add = [&](CCNode* node, float height, float raise) {
      if (!node)
        return;
      node->setPosition({theme.resultsPadding + (inner - node->getContentWidth()) / 2, y - height / 2 - raise});
      card->addChild(node, 1);
      y -= height;
    };

    // The title, the level, GD's words
    ms::Shadow shadow{theme.confirmTitleShadowColor, theme.confirmTitleShadowOpacity, 1,
                      theme.confirmTitleShadowOffset};
    auto title = summary.practice ? (en ? "PRACTICE COMPLETE" : "ПРАКТИКА ПРОЙДЕНА")
                                  : (en ? "LEVEL COMPLETE" : "УРОВЕНЬ ПРОЙДЕН");
    auto titleScale = theme.confirmTitleScale;
    add(line(title, titleScale, theme.confirmTitleColor, theme.titleFont, inner, shadow), TITLE_CAP * titleScale * 1.6f,
        TITLE_RAISE * titleScale);
    if (!summary.level.empty())
      add(line(summary.level, theme.resultsNameScale, theme.resultsNameColor, theme.font, inner),
          FONT_CAP * theme.resultsNameScale * 1.8f, FONT_RAISE * theme.resultsNameScale);
    for (auto const& words : {summary.endText, summary.message})
    {
      if (!words.empty())
        add(line(words, theme.resultsTextScale, theme.resultsTextColor, theme.font, inner),
            FONT_CAP * theme.resultsTextScale * 1.9f, FONT_RAISE * theme.resultsTextScale);
    }
    y -= theme.resultsGap;

    // GD's numbers, and the coins
    auto rows = summary.rows;
    if (summary.coins > 0)
      rows.emplace_back(en ? "Coins" : "Монеты", fmt::format("{} / {}", summary.collected, summary.coins));
    for (auto const& [name, value] : rows)
    {
      auto row = ms::createValueRow(name, value, inner);
      row->setPosition({theme.resultsPadding, y});
      card->addChild(row, 1);
      y -= theme.statsRowHeight + theme.statsRowGap;
    }
    y -= theme.resultsGap;

    // The buttons, each pressing GD's own; GD's hide
    auto buttons = MSButtonMenu::create();
    auto style = MSMenuItem::Style::confirm();
    for (auto const& entry : BUTTONS)
    {
      auto button = typeinfo_cast<CCMenuItem*>(menu->getChildByID(entry.id));
      if (!button || !button->isVisible())
        continue;
      auto item = MSMenuItem::create(en ? entry.en : entry.ru, button,
                                     [button = Ref(button)](MSMenuItem*) {
                                       ms::playSound(Theme::get().clickSound);
                                       button->activate();
                                     },
                                     inner, style);
      if (!item)
        continue;
      button->setVisible(false);
      item->setPosition({theme.resultsPadding, y - style.height / 2});
      buttons->addChild(item);
      y -= style.height + theme.resultsButtonGap;
    }
    card->addChild(buttons, 2);
    auto height = -y + theme.resultsPadding - theme.resultsButtonGap;

    // The plate under it all, a thin pink frame around it
    auto frame = ms::createPanel(theme.rowSprite, theme.resultsFrameColor, 255);
    frame->setAnchorPoint({0, 1});
    frame->ignoreAnchorPointForPosition(false);
    frame->setContentSize({width + 2, height + 2});
    frame->setPosition({-1, 1});
    card->addChild(frame, -2);
    auto plate = ms::createPanel(theme.rowSprite, theme.resultsCardColor, theme.resultsCardOpacity);
    plate->setAnchorPoint({0, 1});
    plate->ignoreAnchorPointForPosition(false);
    plate->setContentSize({width, height});
    card->addChild(plate, -1);

    // On the left, in the middle of the screen's height
    card->setPosition({theme.resultsLeft, winSize.height / 2 + height / 2});
    main->addChild(card, CARD_Z);

    // GD's own layer, hidden under the card
    for (auto id : VANILLA)
    {
      if (auto node = main->getChildByID(id))
        node->setVisible(false);
    }
    for (int i = 1;; i++)
    {
      auto back = main->getChildByID(fmt::format("coin-{}-background", i));
      if (!back)
        break;
      back->setVisible(false);
      if (auto coin = this->getChildByIDRecursive(fmt::format("coin-{}-sprite", i)))
        coin->setVisible(false);
    }
  }
};
