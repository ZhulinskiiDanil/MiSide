#include "Theme.hpp"
#include "levels/LevelPreview.hpp"
#include "levels/MSLevelCard.hpp"
#include "levels/MSLevelList.hpp"
#include "pause/MSPauseColumn.hpp"
#include "ui/MSBackground.hpp"
#include "ui/MSStyle.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/LevelSelectLayer.hpp>

using namespace geode::prelude;

namespace
{
  constexpr int BACKGROUND_Z = -100;
  constexpr int MENU_Z = 20;
  // The official levels' IDs
  constexpr int FIRST_LEVEL = 1;
  constexpr int LAST_LEVEL = 22;
  // The tower's page has this level ID (geode-sdk/NodeIDs, LevelPage.cpp)
  constexpr int TOWER_PAGE = -2;
  // The title font's capitals: 17.5 pt high at scale 1, their middle 2.82 pt over the label's
  constexpr float TITLE_CAP = 17.5f;
  constexpr float CAP_RAISE = 2.82f;

  // The level select's own nodes by Geode's IDs (geode-sdk/NodeIDs, LevelSelectLayer.cpp): hidden while the MiSide
  // screen shows, never removed
  constexpr char const* VANILLA[] = {"background",          "ground-layer", "top-bar-sprite", "bottom-left-corner",
                                     "bottom-right-corner", "levels-list",  "bottom-center-menu", "arrows-menu",
                                     "back-menu",           "info-menu"};

  bool english() { return Mod::get()->getSettingValue<std::string>("menu-language") == "EN"; }

  // Its exit drops the pictures' textures: the screen is gone
  class Lifetime : public CCNode
  {
  public:
    static Lifetime* create()
    {
      auto node = new Lifetime();
      node->init();
      node->autorelease();
      return node;
    }

    void onExit() override
    {
      preview::forget();
      CCNode::onExit();
    }
  };
} // namespace

// The official levels as MiSide's save loading screen: "CHOOSE A LEVEL" over a scrolling list of every official level
// on the left, BACK and MORE under it, a white-pink floppy disk on the right with the chosen level's picture and
// numbers, and PLAY. A row only chooses; PLAY plays through GD's own level page (its lock and everything else as GD
// has it). MORE has the soundtrack, the info, the tower and the vanilla screen (its secret coin and door)
class $modify(MSLevelSelectLayer, LevelSelectLayer)
{
  struct Fields
  {
    bool m_styled = false;
    bool m_more = false;
    bool m_vanilla = false;
    std::vector<Ref<GJGameLevel>> m_levels;
    std::vector<Ref<CCNode>> m_hidden;
    MSLevelList* m_list = nullptr;
    MSLevelCard* m_card = nullptr;
    MSPauseColumn* m_moreColumn = nullptr;
    CCNode* m_title = nullptr;
    CCNode* m_moreTitle = nullptr;
    CCNode* m_background = nullptr;
    CCMenu* m_bottom = nullptr;
    CCMenu* m_back = nullptr;
    unsigned m_closedOnFrame = 0;
  };

  static void onModify(auto& self)
  {
    // After Geode's IDs
    if (self.setHookPriorityPost("LevelSelectLayer::init", Priority::VeryLate).isErr())
      log::warn("Can't set the LevelSelectLayer::init hook's priority");
  }

  bool init(int page)
  {
    if (!LevelSelectLayer::init(page))
      return false;
    if (Mod::get()->getSettingValue<bool>("vanilla-menu") || !Theme::get().levelsEnabled)
      return true;
    this->build(page);
    return true;
  }

  CCMenuItem* button(std::string_view id) { return typeinfo_cast<CCMenuItem*>(this->getChildByIDRecursive(id)); }

  CCNode* title(std::string const& text)
  {
    auto const& theme = Theme::get();
    ms::Shadow shadow{theme.confirmTitleShadowColor, theme.confirmTitleShadowOpacity, 1,
                      theme.confirmTitleShadowOffset};
    auto label = ms::createText(text, theme.confirmTitleScale, theme.confirmTitleColor, theme.titleFont, shadow);
    if (!label.node)
      return nullptr;
    auto capTop = theme.levelsTitleCapTop + CCDirector::get()->getWinSize().height - 320;
    label.node->setPosition(
      {theme.levelsTitleLeft, capTop - (TITLE_CAP / 2 + CAP_RAISE) * theme.confirmTitleScale});
    this->addChild(label.node, MENU_Z);
    return label.node;
  }

  void build(int page)
  {
    auto const& theme = Theme::get();
    auto& f = *m_fields.self();
    auto en = english();
    auto fromTop = CCDirector::get()->getWinSize().height - 320;

    // Every official level, as GD keeps them
    std::vector<std::string> names;
    for (int id = FIRST_LEVEL; id <= LAST_LEVEL; id++)
    {
      auto level = GameLevelManager::get()->getMainLevel(id, true);
      if (!level || level->m_levelID.value() != id)
        continue;
      std::string name = level->m_levelName;
      std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::toupper(c); });
      names.push_back(name);
      f.m_levels.emplace_back(level);
    }
    if (f.m_levels.empty())
      return;

    // The vanilla screen hides, its touches off (its pages would still scroll under ours)
    for (auto id : VANILLA)
    {
      if (auto node = this->getChildByID(id); node && node->isVisible())
      {
        node->setVisible(false);
        f.m_hidden.emplace_back(node);
      }
    }
    if (m_scrollLayer)
      m_scrollLayer->setTouchEnabled(false);

    if (auto background = MSBackground::createPlain(theme.levelsBackdropTop, theme.levelsBackdropBottom))
    {
      this->addChild(background, BACKGROUND_Z);
      f.m_background = background;
    }
    f.m_title = this->title(en ? "CHOOSE A LEVEL" : "ВЫБРАТЬ УРОВЕНЬ");
    f.m_moreTitle = this->title(en ? "MORE" : "ДОПОЛНИТЕЛЬНО");
    if (f.m_moreTitle)
    {
      ms::setOpacity(f.m_moreTitle, 0);
      f.m_moreTitle->setVisible(false);
    }

    f.m_card = MSLevelCard::create([this] { this->play(); });
    if (f.m_card)
      this->addChild(f.m_card, MENU_Z);
    f.m_list = MSLevelList::create(names, [this](int index) {
      if (m_fields->m_card)
        m_fields->m_card->show(m_fields->m_levels[index]);
    });
    if (f.m_list)
    {
      // The row under the mouse shows on the card at once; off the rows, the chosen one comes back
      f.m_list->setOnHover([this](int index) {
        auto& fields = *m_fields.self();
        auto shown = index >= 0 ? index : fields.m_list->chosen();
        if (fields.m_card && shown >= 0 && shown < static_cast<int>(fields.m_levels.size()))
          fields.m_card->show(fields.m_levels[shown]);
      });
      this->addChild(f.m_list, MENU_Z);
    }

    // BACK and MORE under the list, as MiSide's BACK
    auto bottom = CCMenu::create();
    bottom->setPosition({0, 0});
    bottom->setContentSize({0, 0});
    auto style = MSMenuItem::Style::confirm();
    auto backY = theme.levelsBackTop + fromTop - style.height / 2;
    if (auto back = this->button("back-button"))
    {
      if (auto item = MSMenuItem::create(en ? "BACK" : "НАЗАД", nullptr,
                                         [back = Ref(back)](MSMenuItem*) {
                                           ms::playSound(Theme::get().clickSound);
                                           back->activate();
                                         },
                                         theme.levelsBackWidth, style))
      {
        item->setPosition({theme.levelsRowLeft - 3, backY});
        bottom->addChild(item);
      }
    }
    if (auto item = MSMenuItem::create(en ? "MORE" : "ЕЩЁ", nullptr, [this](MSMenuItem*) { this->showMore(true); },
                                       theme.levelsMoreWidth, style))
    {
      item->setPosition({theme.levelsRowLeft - 3 + theme.levelsBackWidth + 4, backY});
      bottom->addChild(item);
    }
    this->addChild(bottom, MENU_Z);
    f.m_bottom = bottom;

    // MORE: what isn't a level, in the list's place
    auto more = MSPauseColumn::create();
    if (auto soundtrack = this->button("download-soundtrack-button"))
      more->addRow(en ? "SOUNDTRACK" : "САУНДТРЕК", [soundtrack = Ref(soundtrack)] { soundtrack->activate(); });
    if (auto info = this->button("info-button"))
      more->addRow(en ? "INFO" : "ИНФОРМАЦИЯ", [info = Ref(info)] { info->activate(); });
    more->addRow(en ? "THE TOWER" : "БАШНЯ", [this] { this->openTower(); });
    more->addRow(en ? "VANILLA SCREEN" : "ИСХОДНЫЙ ЭКРАН", [this] { this->showVanilla(true); });
    more->addRow(en ? "BACK" : "НАЗАД", [this] { this->showMore(false); }, theme.moreCloseSound);
    this->addChild(more, MENU_Z);
    more->finish(theme.levelsRowLeft, theme.levelsFirstRowTop + fromTop);
    more->setShown(false, false);
    f.m_moreColumn = more;

    // Back from the vanilla screen: a small row in the bottom right corner, where no vanilla button is
    auto backToStyled = CCMenu::create();
    backToStyled->setPosition({0, 0});
    backToStyled->setContentSize({0, 0});
    if (auto item = MSMenuItem::create("MISIDE", nullptr, [this](MSMenuItem*) { this->showVanilla(false); }, 48,
                                       MSMenuItem::Style::pause()))
    {
      auto winSize = CCDirector::get()->getWinSize();
      item->setPosition({winSize.width - 56, winSize.height - 8 - item->getContentHeight() / 2});
      backToStyled->addChild(item);
    }
    backToStyled->setVisible(false);
    this->addChild(backToStyled, MENU_Z);
    f.m_back = backToStyled;

    this->addChild(Lifetime::create());
    f.m_styled = true;
    // The page GD opened on (the last level played), else the first
    if (f.m_list)
      f.m_list->choose(page >= 0 && page < static_cast<int>(f.m_levels.size()) ? page : 0);
  }

  // ! --- Actions --- !

  LevelPage* anyPage()
  {
    if (!m_scrollLayer || !m_scrollLayer->m_pages)
      return nullptr;
    for (auto node : CCArrayExt<CCNode*>(m_scrollLayer->m_pages))
    {
      if (auto page = typeinfo_cast<LevelPage*>(node))
        return page;
    }
    return nullptr;
  }

  // GD's own level page, given the chosen level, presses its own play: the lock, the transition, every mod's hook
  void play()
  {
    auto& f = *m_fields.self();
    if (!f.m_list || f.m_more || f.m_vanilla)
      return;
    auto index = f.m_list->chosen();
    if (index < 0 || index >= static_cast<int>(f.m_levels.size()))
      return;
    auto page = this->anyPage();
    if (!page)
    {
      log::warn("Level select: no level page to play from");
      return;
    }
    this->updatePageWithObject(page, f.m_levels[index]);
    page->onPlay(page->getChildByIDRecursive("level-button"));
  }

  void openTower()
  {
    if (auto page = this->anyPage())
      page->onTheTower(nullptr);
  }

  // ! --- States --- !

  void fade(CCNode* node, bool shown)
  {
    if (!node)
      return;
    auto time = std::max(Theme::get().pauseDuration, 0.f);
    node->stopAllActions();
    if (shown)
    {
      node->setVisible(true);
      node->runAction(CCFadeTo::create(time, 255));
    }
    else
      node->runAction(CCSequence::createWithTwoActions(CCFadeTo::create(time, 0), CCHide::create()));
  }

  void showMore(bool shown)
  {
    auto& f = *m_fields.self();
    if (!f.m_styled || f.m_vanilla || f.m_more == shown)
      return;
    if (shown)
      ms::playSound(Theme::get().moreOpenSound);
    f.m_more = shown;
    if (f.m_list)
      f.m_list->setShown(!shown);
    f.m_moreColumn->setShown(shown);
    this->fade(f.m_title, !shown);
    this->fade(f.m_moreTitle, shown);
    for (auto child : CCArrayExt<MSMenuItem*>(f.m_bottom->getChildren()))
    {
      child->setEnabled(!shown);
      child->resetLook();
      child->fadeTo(std::max(Theme::get().pauseDuration, 0.f), shown ? 0 : 255);
    }
    if (f.m_card)
      f.m_card->setShown(!shown);
    if (!shown)
      f.m_closedOnFrame = CCDirector::get()->getTotalFrames();
  }

  // The vanilla screen as it was (its secret coin, its door); back with the small row
  void showVanilla(bool shown)
  {
    auto& f = *m_fields.self();
    if (!f.m_styled || f.m_vanilla == shown)
      return;
    if (shown && f.m_more)
    {
      f.m_moreColumn->setShown(false, false);
      f.m_more = false;
    }
    f.m_vanilla = shown;
    for (auto node : {f.m_background, f.m_title, static_cast<CCNode*>(f.m_list), static_cast<CCNode*>(f.m_card),
                      static_cast<CCNode*>(f.m_bottom)})
    {
      if (node)
        node->setVisible(!shown);
    }
    if (f.m_moreTitle)
      f.m_moreTitle->setVisible(false);
    if (!shown)
    {
      if (f.m_title)
        ms::setOpacity(f.m_title, 255);
      if (f.m_list)
        f.m_list->setShown(true);
      if (f.m_card)
        f.m_card->setShown(true);
      for (auto child : CCArrayExt<MSMenuItem*>(f.m_bottom->getChildren()))
      {
        child->setEnabled(true);
        child->fadeTo(0, 255);
      }
    }
    for (auto const& node : f.m_hidden)
      node->setVisible(shown);
    if (m_scrollLayer)
      m_scrollLayer->setTouchEnabled(shown);
    f.m_back->setVisible(shown);
  }

  // ! --- Keys --- !

  void keyDown(enumKeyCodes key, double timestamp)
  {
    auto& f = *m_fields.self();
    if (!f.m_styled || f.m_vanilla)
      return LevelSelectLayer::keyDown(key, timestamp);
    auto frame = CCDirector::get()->getTotalFrames();
    auto back = key == KEY_Escape || key == CONTROLLER_B;
    if (back && f.m_closedOnFrame == frame)
      return;
    if (f.m_more)
    {
      if (back)
      {
        ms::playSound(Theme::get().moreCloseSound);
        return this->showMore(false);
      }
      if (f.m_moreColumn->onKey(key))
        return;
    }
    else
    {
      if (f.m_list && f.m_list->onKey(key))
        return;
      if (f.m_card && f.m_card->onKey(key))
        return;
      // GD's page keys would turn its hidden pages
      if (key == KEY_Left || key == KEY_Right || key == CONTROLLER_Left || key == CONTROLLER_Right)
        return;
    }
    LevelSelectLayer::keyDown(key, timestamp);
  }

  void keyBackClicked()
  {
    auto& f = *m_fields.self();
    if (f.m_styled && !f.m_vanilla)
    {
      if (f.m_closedOnFrame == CCDirector::get()->getTotalFrames())
        return;
      if (f.m_more)
      {
        ms::playSound(Theme::get().moreCloseSound);
        return this->showMore(false);
      }
    }
    LevelSelectLayer::keyBackClicked();
  }
};
