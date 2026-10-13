#include "Theme.hpp"
#include "pause/MSPauseColumn.hpp"
#include "ui/MSBackground.hpp"
#include "ui/MSLogo.hpp"
#include "ui/MSMita.hpp"
#include "ui/MSModTray.hpp"
#include "ui/MSScenes.hpp"
#include "ui/MSStyle.hpp"
#include "ui/MSWave.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/CreatorLayer.hpp>

using namespace geode::prelude;

namespace
{
  constexpr int BACKGROUND_Z = -100;
  constexpr int MITA_Z = -50;
  constexpr int MENU_Z = 10;

  // A vanilla button of GD's creator screen (Geode's Node IDs) and its row
  struct Entry
  {
    char const* id;
    char const* ru;
    char const* en;
  };

  struct Group
  {
    char const* ru;
    char const* en;
    std::vector<Entry> entries;
  };

  std::vector<Group> const& groups()
  {
    static std::vector<Group> const GROUPS = {
      {"МОИ УРОВНИ", "MY LEVELS", {{"create-button", "СОЗДАТЬ УРОВЕНЬ", "CREATE"}, {"saved-button", "СОХРАНЁННЫЕ", "SAVED"}}},
      {"ОНЛАЙН",
       "ONLINE",
       {{"featured-button", "ИЗБРАННОЕ", "FEATURED"},
        {"lists-button", "СПИСКИ", "LISTS"},
        {"search-button", "ПОИСК", "SEARCH"},
        {"map-packs-button", "НАБОРЫ УРОВНЕЙ", "MAP PACKS"},
        {"gauntlets-button", "ИСПЫТАНИЯ", "GAUNTLETS"},
        {"paths-button", "ПУТИ", "PATHS"},
        {"scores-button", "РЕЙТИНГИ", "SCORES"}}},
      {"КАЖДЫЙ ДЕНЬ",
       "EVERY DAY",
       {{"daily-button", "ЕЖЕДНЕВНЫЙ УРОВЕНЬ", "DAILY LEVEL"},
        {"weekly-button", "ЕЖЕНЕДЕЛЬНЫЙ ДЕМОН", "WEEKLY DEMON"},
        {"event-button", "СОБЫТИЕ", "EVENT LEVEL"},
        {"quests-button", "КВЕСТЫ", "QUESTS"}}},
      {"ЕЩЁ",
       "MORE",
       {{"map-button", "КАРТА", "THE MAP"},
        {"versus-button", "ДУЭЛЬ", "VERSUS"},
        {"vault-button", "ХРАНИЛИЩЕ", "THE VAULT"},
        {"treasure-room-button", "СОКРОВИЩНИЦА", "TREASURE ROOM"}}},
    };
    return GROUPS;
  }

  // The vanilla screen's look, hidden as the column takes its place (never removed)
  constexpr char const* VANILLA[] = {"background", "bottom-left-corner", "top-left-corner"};
  // The menus GD's buttons are in: what's left in them (the mods' buttons) goes in the tray
  char const* const TRAY_SOURCES[] = {"creator-buttons-menu", "top-right-menu", "bottom-right-menu", "exit-menu",
                                      "bottom-left-menu"};

  bool english() { return Mod::get()->getSettingValue<std::string>("menu-language") == "EN"; }

  // The arrow keys, Enter and the controller's for the column: GD gives the keys to the last delegate only, so a
  // child of the screen added after it has them
  class MSCreatorKeys : public CCLayer
  {
  public:
    static MSCreatorKeys* create(MSPauseColumn* column)
    {
      auto keys = new MSCreatorKeys();
      keys->m_column = column;
      keys->init();
      keys->autorelease();
      return keys;
    }

    bool init() override
    {
      if (!CCLayer::init())
        return false;
      this->setKeyboardEnabled(true);
      return true;
    }

    void keyDown(enumKeyCodes key, double) override
    {
      if (m_column)
        m_column->onKey(key);
    }

    MSPauseColumn* m_column = nullptr;
  };
} // namespace

// GD's creator screen (CREATE) as a page of MiSide's: the menu's background, Mita and logo, CREATE where MAIN MENU
// is, GD's buttons as rows in groups, BACK under them. Each row presses GD's own button (hidden, never removed), so
// its lock, its popups and the mods hooking it work as in GD; the buttons other mods add go in the tray. It comes in
// from the left as a page does once the scene's transition is over
class $modify(MSCreatorLayer, CreatorLayer)
{
  struct Fields
  {
    bool m_styled = false;
    bool m_leaving = false;
    MSPauseColumn* m_column = nullptr;
    CCNode* m_title = nullptr;
  };

  static void onModify(auto& self)
  {
    // After Geode's IDs
    if (self.setHookPriorityPost("CreatorLayer::init", Priority::VeryLate).isErr())
      log::warn("Can't set the CreatorLayer::init hook's priority");
  }

  bool init()
  {
    if (!CreatorLayer::init())
      return false;
    if (!Mod::get()->getSettingValue<bool>("vanilla-menu"))
      this->build();
    return true;
  }

  CCMenuItem* button(char const* id) { return typeinfo_cast<CCMenuItem*>(this->getChildByIDRecursive(id)); }

  void build()
  {
    auto const& theme = Theme::get();
    auto& f = *m_fields.self();
    auto en = english();
    auto fromTop = CCDirector::get()->getWinSize().height - 320;

    auto column = MSPauseColumn::create();
    if (!column)
      return;
    column->setRowWidth(theme.optionsRowWidth);
    auto rows = 0;
    for (auto const& group : groups())
    {
      std::vector<std::pair<Entry const*, Ref<CCMenuItem>>> found;
      for (auto const& entry : group.entries)
      {
        if (auto target = this->button(entry.id))
          found.emplace_back(&entry, target);
      }
      if (found.empty())
        continue;
      column->addHeader(en ? group.en : group.ru);
      for (auto const& [entry, target] : found)
      {
        column->addRow(en ? entry->en : entry->ru, [target] { target->activate(); });
        rows++;
      }
    }
    if (rows == 0)
    {
      log::warn("Creator: none of GD's buttons found, GD's own screen");
      return;
    }
    // BACK under them, as the pause's Quit: GD's own way back
    column->setExit(en ? "BACK" : "НАЗАД", [this] { this->onBack(nullptr); }, theme.moreCloseSound);
    column->setViewHeight(theme.screenTop - theme.screenBottom - theme.pauseRowPitch - theme.pauseExitGap);

    // GD's look hidden, its buttons too: the rows press them
    for (auto id : VANILLA)
    {
      if (auto node = this->getChildByID(id))
        node->setVisible(false);
    }
    for (auto const& group : groups())
    {
      for (auto const& entry : group.entries)
      {
        if (auto target = this->button(entry.id))
          target->setVisible(false);
      }
    }
    if (auto exit = this->button("exit-button"))
      exit->setVisible(false);

    auto strength = static_cast<float>(Mod::get()->getSettingValue<double>("parallax-strength"));
    if (auto background = MSBackground::create(strength))
    {
      background->setID("background"_spr);
      this->addChild(background, BACKGROUND_Z);
    }
    if (auto mita = MSMita::create())
    {
      mita->setID("mita"_spr);
      this->addChild(mita, MITA_Z);
    }
    if (Mod::get()->getSettingValue<bool>("show-title"))
    {
      if (auto logo = MSLogo::create())
      {
        logo->fit(1);
        logo->ignoreAnchorPointForPosition(false);
        logo->setAnchorPoint({0, 0});
        logo->setPosition({theme.logoLeft, theme.logoBottom + fromTop});
        this->addChild(logo, MENU_Z);
      }
    }
    // CREATE where MAIN MENU is, as it looks
    if (auto title = ms::createText(en ? "CREATE" : "СОЗДАТЬ", theme.subtitleScale, theme.subtitleColor,
                                    theme.titleFont)
                       .node)
    {
      title->setAnchorPoint({0, 1});
      title->setPosition({theme.columnLeft, theme.subtitleTop + fromTop});
      this->addChild(title, MENU_Z);
      f.m_title = title;
    }

    column->setID("column"_spr);
    this->addChild(column, MENU_Z);
    column->finish(theme.screenLeft, theme.screenTop + fromTop);
    f.m_column = column;
    this->addChild(MSCreatorKeys::create(column));

    // The mods' buttons: what's still shown in GD's menus
    if (auto tray = MSModTray::create({std::begin(TRAY_SOURCES), std::end(TRAY_SOURCES)}, nullptr))
    {
      tray->setID("mod-tray"_spr);
      this->addChild(tray, MENU_Z);
    }

    // Out of sight until the scene is in: then from the left, as a page comes
    ms::wave::hide(this->pageSteps());
    ms::scenes::whenShown(this, [this] { ms::wave::in(this->pageSteps()); });
    f.m_styled = true;
  }

  ms::wave::Steps pageSteps()
  {
    auto& f = *m_fields.self();
    ms::wave::Steps steps;
    if (f.m_title)
      steps.push_back({{f.m_title}});
    if (f.m_column)
    {
      auto rows = f.m_column->pageSteps();
      steps.insert(steps.end(), rows.begin(), rows.end());
    }
    return steps;
  }

  // BACK, Esc: the page out, then the menu at once (no fade through black), its page coming in
  void onBack(CCObject* sender)
  {
    auto& f = *m_fields.self();
    if (!f.m_styled)
      return CreatorLayer::onBack(sender);
    if (f.m_leaving)
      return;
    f.m_leaving = true;
    auto gone = ms::wave::out(this->pageSteps());
    Ref<CreatorLayer> self = this;
    this->runAction(CCSequence::createWithTwoActions(CCDelayTime::create(gone), CallFuncExt::create([self] {
      ms::scenes::skipNextTransition();
      static_cast<MSCreatorLayer*>(self.data())->CreatorLayer::onBack(nullptr);
    })));
  }
};
