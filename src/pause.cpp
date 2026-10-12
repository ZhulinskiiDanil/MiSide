#include "Sounds.hpp"
#include "Theme.hpp"
#include "pause/MSPauseAmbient.hpp"
#include "pause/MSPauseBackdrop.hpp"
#include "pause/MSPauseColumn.hpp"
#include "pause/MSPauseInfo.hpp"
#include "ui/MSConfirm.hpp"
#include "ui/MSStyle.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>

using namespace geode::prelude;

namespace
{
  // Under everything of the pause menu: the blurred level
  constexpr int BACKDROP_Z = -1000;
  // Over the vanilla nodes (Geode's IDs put the mods' menus at 10)
  constexpr int MENU_Z = 20;
  constexpr int CONFIRM_Z = 1000;
  constexpr int FADE_ACTION = 0x4d59;
  // GD's exit confirmation option: PauseLayer::tryQuit asks only when it's on
  constexpr char const* CONFIRM_EXIT = "0167";
  // The title font's capitals: 17.5 pt high at scale 1, their middle 2.82 pt over the label's
  constexpr float TITLE_CAP = 17.5f;
  constexpr float CAP_RAISE = 2.82f;
  // MiSide's confirmation: 5.1 pt from the title's line to the first button
  constexpr float CONFIRM_GAP = 5.1f;

  // The pause menu's own nodes by Geode's IDs (geode-sdk/NodeIDs, PauseLayer.cpp): replaced, so hidden while the
  // MiSide pause shows. Never removed; anything not named here is left alone
  constexpr char const* VANILLA[] = {
    "background",           "level-name",           "normal-progress-bar",     "practice-progress-bar",
    "normal-progress-label", "practice-progress-label", "normal-mode-label",   "practice-mode-label",
    "play-time",            "points-label",         "center-button-menu",      "practice-arrow-text",
    "music-slider",         "sfx-slider",           "music-label",             "sfx-label",
    "controller-play-hint", "controller-back-hint", "controller-practice-hint",
  };
  // The vanilla buttons of center-button-menu; anything else there is a mod's, shown under More
  constexpr char const* CENTER_BUTTONS[] = {"edit-button",     "full-restart-button", "exit-button",
                                            "practice-button", "play-button",         "retry-button"};

  bool english() { return Mod::get()->getSettingValue<std::string>("menu-language") == "EN"; }

  std::vector<std::string> split(std::string const& list)
  {
    std::vector<std::string> parts;
    std::string part;
    for (auto c : list + ",")
    {
      if (c != ',')
      {
        part += c;
        continue;
      }
      auto start = part.find_first_not_of(' ');
      auto end = part.find_last_not_of(' ');
      if (start != std::string::npos)
        parts.push_back(part.substr(start, end - start + 1));
      part.clear();
    }
    return parts;
  }

  // A label's middle so that its capitals' top is at capTop (the title font)
  float titleMiddle(float capTop, float scale) { return capTop - (TITLE_CAP / 2 + CAP_RAISE) * scale; }

  // Its exit stops the ambient: the pause menu is gone (resumed, restarted, left)
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
      MSPauseAmbient::get()->stop();
      CCNode::onExit();
    }
  };
} // namespace

// MiSide's pause: the level blurred and tinted behind, "PAUSE" with a column of see-through rows on the left
// (Resume, Restart, Practice, the music and effects volumes, Settings, More, and Quit apart), the level's name and
// numbers in a bar on the right. Every row presses its vanilla button with activate(), so GD's handlers and every
// mod's hooks on them run as usual. More shows the mods' pause buttons where they are, untouched, and can bring back
// the vanilla pause. Quit asks in the column, as the main menu does; the confirmation also works with the MiSide
// pause off (in place of GD's popup)
class $modify(MSPauseLayer, PauseLayer)
{
  enum class State
  {
    Main,
    More,
    Confirm,
    Vanilla,
  };

  struct Fields
  {
    bool m_styled = false;
    State m_state = State::Main;
    MSPauseBackdrop* m_backdrop = nullptr;
    CCNode* m_title = nullptr;
    CCNode* m_subtitle = nullptr;
    CCNode* m_moreTitle = nullptr;
    MSPauseInfo* m_info = nullptr;
    MSPauseColumn* m_column = nullptr;
    MSPauseColumn* m_more = nullptr;
    MSConfirm* m_confirm = nullptr;
    CCMenu* m_back = nullptr;
    // The vanilla nodes hidden, to show again for the vanilla pause
    std::vector<Ref<CCNode>> m_vanilla;
    // The mods' menus, shown under More
    std::vector<Ref<CCNode>> m_modMenus;
    // center-button-menu's own buttons, hidden while it shows the mods' buttons in it under More
    std::vector<Ref<CCNode>> m_centerOwn;
    bool m_centerHasMods = false;
    GLubyte m_ownOpacity = 0;
    // With the MiSide pause off: what the confirmation hid
    std::vector<Ref<CCNode>> m_hidden;
    // GD sends Esc both as a key (keyDown) and as back (keyBackClicked): one press acts once
    unsigned m_closedOnFrame = 0;
    unsigned m_openedOnFrame = 0;
    bool m_quitting = false;
  };

  static void onModify(auto& self)
  {
    // After Geode's IDs and the other mods' customSetup: their buttons are in their menus by then
    if (self.setHookPriorityPost("PauseLayer::customSetup", Priority::VeryLate).isErr())
      log::warn("Can't set the PauseLayer::customSetup hook's priority");
  }

  void customSetup()
  {
    PauseLayer::customSetup();
    if (Mod::get()->getSettingValue<bool>("vanilla-menu"))
      return;
    Theme::reload();
    if (Theme::get().pauseEnabled)
      this->buildStyled();
    this->addConfirm();
    if (m_fields->m_styled)
    {
      this->addChild(Lifetime::create());
      auto const& theme = Theme::get();
      auto ambient = MSPauseAmbient::get();
      ambient->playOnce(theme.pauseStartSound, theme.pauseStartVolume);
      // Stress near the personal best, else the usual ambient
      auto stress = sounds::stressLevel();
      if (theme.pauseStressEnabled && stress > 0)
        ambient->start(theme.pauseStressPath, theme.pauseStressVolume * stress);
      else if (theme.pauseAmbientEnabled)
        ambient->start(theme.pauseAmbientPath, theme.pauseAmbientVolume);
    }
  }

  // ! --- Building --- !

  CCMenuItem* button(std::string_view id) { return typeinfo_cast<CCMenuItem*>(this->getChildByIDRecursive(id)); }

  void buildStyled()
  {
    auto const& theme = Theme::get();
    auto& f = *m_fields.self();
    auto en = english();
    auto level = PlayLayer::get();
    if (!level || !level->m_level)
      return;
    // MiSide's screen is 320 high: positions from the top
    auto fromTop = CCDirector::get()->getWinSize().height - 320;

    // What's shown of the vanilla buttons before they hide (an unavailable one gets no row)
    auto available = [this](std::string_view id) -> CCMenuItem* {
      auto target = this->button(id);
      return target && target->isVisible() && target->isEnabled() ? target : nullptr;
    };
    auto press = [](CCMenuItem* target) { return [target = Ref(target)] { target->activate(); }; };

    // The background: the blurred level, tinted. GD's own darkening (the layer's colour) would come over it
    auto backdrop = MSPauseBackdrop::create(this);
    if (!backdrop)
      return;
    backdrop->setID("pause-backdrop"_spr);
    this->addChild(backdrop, BACKDROP_Z);
    f.m_backdrop = backdrop;
    f.m_ownOpacity = this->getOpacity();
    this->setOpacity(0);

    // The rows, while the vanilla buttons still show what's available
    auto column = MSPauseColumn::create();
    if (auto target = available("play-button"))
      column->addRow(en ? "RESUME" : "ПРОДОЛЖИТЬ", press(target));
    if (auto target = available("retry-button"))
      column->addRow(en ? "RESTART" : "ЗАНОВО", press(target));
    if (auto target = available("practice-button"))
    {
      auto text = level->m_isPracticeMode ? (en ? "NORMAL MODE" : "ОБЫЧНЫЙ РЕЖИМ") : (en ? "PRACTICE" : "ПРАКТИКА");
      column->addRow(text, press(target));
    }
    this->addVolume(column, "music-slider", en ? "MUSIC" : "МУЗЫКА", true, theme.pauseMusicReset);
    this->addVolume(column, "sfx-slider", en ? "SFX" : "ЗВУКИ", false, theme.pauseSfxReset);
    if (auto target = available("options-button"))
      column->addRow(en ? "SETTINGS" : "НАСТРОЙКИ", press(target));
    column->addRow(en ? "MORE" : "ДОПОЛНИТЕЛЬНО", [this] { this->showMore(true); }, theme.moreOpenSound);
    if (auto target = available("exit-button"))
      column->setExit(en ? "QUIT THE LEVEL" : "ВЫЙТИ ИЗ УРОВНЯ", press(target));
    column->setID("pause-column"_spr);
    this->addChild(column, MENU_Z);
    column->finish(theme.pauseLeft, theme.pauseFirstRowTop + fromTop);
    f.m_column = column;

    // Mods' buttons in the vanilla buttons' own menu, before it hides
    if (auto center = this->getChildByID("center-button-menu"))
    {
      for (auto child : CCArrayExt<CCNode*>(center->getChildren()))
      {
        if (!child->isVisible())
          continue;
        auto own = std::find(std::begin(CENTER_BUTTONS), std::end(CENTER_BUTTONS), child->getID()) !=
                   std::end(CENTER_BUTTONS);
        if (own)
          f.m_centerOwn.emplace_back(child);
        else
          f.m_centerHasMods = true;
      }
    }
    // The vanilla pause's nodes hide (never removed)
    for (auto id : VANILLA)
    {
      if (auto node = this->getChildByID(id); node && node->isVisible())
      {
        node->setVisible(false);
        f.m_vanilla.emplace_back(node);
      }
    }
    if (theme.pauseExtraEnabled)
    {
      for (auto const& id : split(theme.pauseExtraMenus))
      {
        if (auto node = this->getChildByID(id); node && node->isVisible())
        {
          node->setVisible(false);
          f.m_modMenus.emplace_back(node);
        }
      }
    }

    // The titles
    auto title = ms::createText(en ? "PAUSE" : "ПАУЗА", theme.pauseTitleScale, theme.pauseTitleColor, theme.titleFont,
                                ms::Shadow{});
    if (title.node)
    {
      title.node->setPosition(
        {theme.pauseTitleLeft, titleMiddle(theme.pauseTitleCapTop + fromTop, theme.pauseTitleScale)});
      this->addChild(title.node, MENU_Z);
      f.m_title = title.node;
    }
    for (auto [text, field] : {std::pair{en ? "MENU" : "МЕНЮ", &f.m_subtitle},
                               std::pair{en ? "MORE" : "ДОПОЛНИТЕЛЬНО", &f.m_moreTitle}})
    {
      auto subtitle =
        ms::createText(text, theme.pauseSubtitleScale, theme.pauseSubtitleColor, theme.titleFont, ms::Shadow{});
      if (!subtitle.node)
        continue;
      subtitle.node->setPosition(
        {theme.pauseTitleLeft, titleMiddle(theme.pauseSubtitleCapTop + fromTop, theme.pauseSubtitleScale)});
      this->addChild(subtitle.node, MENU_Z);
      *field = subtitle.node;
    }
    if (f.m_moreTitle)
    {
      ms::setOpacity(f.m_moreTitle, 0);
      f.m_moreTitle->setVisible(false);
    }

    // The level's bar and its numbers
    if (auto info = MSPauseInfo::create(level->m_level->m_levelName, this->stats(level)))
    {
      info->setID("pause-info"_spr);
      this->addChild(info, MENU_Z);
      f.m_info = info;
    }

    // More: the vanilla buttons with no row, the vanilla pause, back. Right of the mods' left menu
    auto more = MSPauseColumn::create();
    if (auto target = available("edit-button"))
      more->addRow(en ? "EDIT" : "ПРАВКА", press(target));
    if (auto target = available("full-restart-button"))
      more->addRow(en ? "FULL RESTART" : "ПОЛНЫЙ ПЕРЕЗАПУСК", press(target));
    more->addRow(en ? "VANILLA PAUSE" : "ИСХОДНАЯ ПАУЗА", [this] { this->showVanilla(true); });
    // Back apart under them, as Quit is under the pause's rows
    more->setExit(en ? "BACK" : "НАЗАД", [this] { this->showMore(false); }, theme.moreCloseSound);
    more->setID("pause-more"_spr);
    this->addChild(more, MENU_Z);
    more->finish(this->moreLeft(), theme.pauseFirstRowTop + fromTop);
    more->setShown(false, false);
    f.m_more = more;

    // Back from the vanilla pause: a small row in the bottom left corner, where no vanilla menu is
    auto back = CCMenu::create();
    back->setPosition({0, 0});
    back->setContentSize({0, 0});
    if (auto item = MSMenuItem::create("MISIDE", nullptr, [this](MSMenuItem*) { this->showVanilla(false); }, 48,
                                       MSMenuItem::Style::pause()))
    {
      item->setPosition({8, 8 + theme.pauseRowHeight / 2});
      back->addChild(item);
    }
    back->setVisible(false);
    back->setID("pause-back"_spr);
    this->addChild(back, MENU_Z);
    f.m_back = back;

    f.m_styled = true;
  }

  // A volume row driving the vanilla slider: its value set, then its own handler with its thumb, as a drag would
  void addVolume(MSPauseColumn* column, std::string_view id, std::string const& text, bool music, float reset)
  {
    auto slider = typeinfo_cast<Slider*>(this->getChildByID(id));
    if (!slider || !slider->isVisible() || !slider->getThumb())
      return;
    MSPauseColumn::Slider row;
    row.get = [music] {
      auto engine = FMODAudioEngine::get();
      return music ? engine->m_musicVolume : engine->m_sfxVolume;
    };
    row.set = [this, slider = Ref(slider), music](float value) {
      slider->setValue(value);
      if (music)
        this->musicSliderChanged(slider->getThumb());
      else
        this->sfxSliderChanged(slider->getThumb());
    };
    row.reset = reset >= 0 ? std::clamp(reset, 0.f, 100.f) / 100 : -1;
    column->addSlider(text, std::move(row));
  }

  std::vector<std::pair<std::string, std::string>> stats(PlayLayer* level)
  {
    auto en = english();
    std::vector<std::pair<std::string, std::string>> stats;
    if (level->m_level->isPlatformer())
    {
      // What GD's own pause shows for a platformer: its time, and the points if it has them
      if (auto time = typeinfo_cast<CCLabelBMFont*>(this->getChildByID("play-time")))
        stats.emplace_back(en ? "TIME" : "ВРЕМЯ", time->getString());
      if (auto points = typeinfo_cast<CCLabelBMFont*>(this->getChildByID("points-label")))
      {
        std::string text = points->getString();
        auto colon = text.find(':');
        stats.emplace_back(en ? "POINTS" : "ОЧКИ", colon == std::string::npos ? text : text.substr(colon + 1));
      }
      return stats;
    }
    // Where the attempt is, and each mode's best on its own
    stats.emplace_back(en ? "NOW" : "СЕЙЧАС", fmt::format("{}%", level->getCurrentPercentInt()));
    stats.emplace_back(en ? "NORMAL" : "ОБЫЧНЫЙ", fmt::format("{}%", level->m_level->m_normalPercent.value()));
    stats.emplace_back(en ? "PRACTICE" : "ПРАКТИКА", fmt::format("{}%", level->m_level->m_practicePercent));
    return stats;
  }

  // More's rows go right of the mods' left menu, when it has buttons
  float moreLeft()
  {
    auto left = Theme::get().pauseLeft;
    for (auto const& menu : m_fields->m_modMenus)
    {
      if (menu->getID() != "left-button-menu")
        continue;
      for (auto child : CCArrayExt<CCNode*>(menu->getChildren()))
      {
        if (!child->isVisible())
          continue;
        auto box =
          CCRectApplyAffineTransform(CCRect{CCPointZero, child->getContentSize()}, child->nodeToWorldTransform());
        left = std::max(left, box.getMaxX() + 6);
      }
    }
    return left;
  }

  // ! --- States --- !

  void fade(CCNode* node, bool shown)
  {
    if (!node)
      return;
    auto time = std::max(Theme::get().pauseDuration, 0.f);
    node->stopActionByTag(FADE_ACTION);
    CCAction* action = nullptr;
    if (shown)
    {
      node->setVisible(true);
      action = CCFadeTo::create(time, 255);
    }
    else
      action = CCSequence::createWithTwoActions(CCFadeTo::create(time, 0), CCHide::create());
    action->setTag(FADE_ACTION);
    node->runAction(action);
  }

  void setModMenus(bool shown)
  {
    for (auto const& menu : m_fields->m_modMenus)
      menu->setVisible(shown);
    // Mods' buttons in center-button-menu: alone in it, its own buttons hidden (its layout closes the gaps)
    if (!m_fields->m_centerHasMods)
      return;
    auto center = this->getChildByID("center-button-menu");
    if (!center)
      return;
    for (auto const& own : m_fields->m_centerOwn)
      own->setVisible(!shown);
    center->setVisible(shown);
    center->updateLayout();
  }

  void showMore(bool shown)
  {
    auto& f = *m_fields.self();
    if (!f.m_styled || f.m_state != (shown ? State::Main : State::More))
      return;
    f.m_state = shown ? State::More : State::Main;
    f.m_column->setShown(!shown);
    f.m_more->setShown(shown);
    this->fade(f.m_subtitle, !shown);
    this->fade(f.m_moreTitle, shown);
    this->fade(f.m_info, !shown);
    this->setModMenus(shown);
    if (!shown)
      f.m_closedOnFrame = CCDirector::get()->getTotalFrames();
  }

  // The vanilla pause as it was, for what doesn't work in the MiSide one; back with the small row
  void showVanilla(bool shown)
  {
    auto& f = *m_fields.self();
    if (!f.m_styled || f.m_state != (shown ? State::More : State::Vanilla))
      return;
    if (shown)
    {
      f.m_more->setShown(false, false);
      if (f.m_moreTitle)
        f.m_moreTitle->setVisible(false);
      this->setModMenus(false);
    }
    f.m_state = shown ? State::Vanilla : State::Main;
    for (auto node : {static_cast<CCNode*>(f.m_backdrop), f.m_title, f.m_subtitle, static_cast<CCNode*>(f.m_info)})
    {
      if (!node)
        continue;
      node->stopActionByTag(FADE_ACTION);
      node->setVisible(!shown);
      ms::setOpacity(node, 255);
    }
    f.m_column->setShown(!shown, false);
    for (auto const& node : f.m_vanilla)
      node->setVisible(shown);
    for (auto const& menu : f.m_modMenus)
      menu->setVisible(shown);
    this->setOpacity(shown ? f.m_ownOpacity : 0);
    f.m_back->setVisible(shown);
  }

  // ! --- Quitting --- !

  void addConfirm()
  {
    auto const& theme = Theme::get();
    auto en = english();
    MSConfirm::Texts texts{en ? "QUIT THE LEVEL?" : "ВЫЙТИ ИЗ УРОВНЯ?", en ? "YES" : "ДА", en ? "NO" : "НЕТ"};
    MSConfirm* confirm = nullptr;
    auto yes = [this] { this->quitLevel(); };
    auto no = [this] { this->closeConfirm(); };
    if (m_fields->m_styled)
    {
      // In the column, under "PAUSE", where "MENU" and the list were; the pause's own rows
      auto capTop = theme.pauseSubtitleCapTop + CCDirector::get()->getWinSize().height - 320;
      MSConfirm::Layout layout{theme.pauseTitleLeft,
                               capTop,
                               theme.confirmTitleScale,
                               theme.pauseLeft,
                               capTop - TITLE_CAP * theme.confirmTitleScale - CONFIRM_GAP,
                               theme.pauseRowPitch,
                               theme.pauseRowWidth};
      confirm = MSConfirm::create(texts, yes, no, layout, MSMenuItem::Style::pause());
    }
    else
      confirm = MSConfirm::create(texts, yes, no);
    if (!confirm)
      return;
    confirm->setID("quit-confirm"_spr);
    this->addChild(confirm, CONFIRM_Z);
    m_fields->m_confirm = confirm;
  }

  void tryQuit(CCObject* sender)
  {
    // The vanilla pause: GD's own way
    if (m_fields->m_state == State::Vanilla)
      return PauseLayer::tryQuit(sender);
    // GD asks only with its option on; the mod's setting asks always. Neither: it leaves at once, as it always did
    auto ask = Mod::get()->getSettingValue<bool>("confirm-level-exit") ||
               GameManager::get()->getGameVariable(CONFIRM_EXIT);
    if (!ask || !this->openConfirm())
      return PauseLayer::tryQuit(sender);
  }

  bool openConfirm()
  {
    auto& f = *m_fields.self();
    if (f.m_quitting)
      return true;
    // None (its text couldn't be made): GD's popup
    auto confirm = f.m_confirm;
    if (!confirm)
      return false;
    if (confirm->isOpen())
      return true;
    if (f.m_styled)
    {
      if (f.m_state == State::More)
        this->showMore(false);
      f.m_state = State::Confirm;
      f.m_column->setShown(false);
      this->fade(f.m_subtitle, false);
    }
    else
      this->hideForConfirm();
    confirm->open();
    f.m_openedOnFrame = CCDirector::get()->getTotalFrames();
    return true;
  }

  // With the MiSide pause off: the vanilla pause's nodes hide, its background (and other mods' blur) stays
  void hideForConfirm()
  {
    auto winSize = CCDirector::get()->getWinSize();
    // Buttons or text somewhere in it: a menu as big as the screen (cocos makes them so) isn't a background
    std::function<bool(CCNode*)> hasContent = [&](CCNode* node) {
      if (typeinfo_cast<CCMenu*>(node) || typeinfo_cast<CCMenuItem*>(node) || typeinfo_cast<CCLabelProtocol*>(node))
        return true;
      for (auto child : CCArrayExt<CCNode*>(node->getChildren()))
      {
        if (hasContent(child))
          return true;
      }
      return false;
    };
    m_fields->m_hidden.clear();
    for (auto child : CCArrayExt<CCNode*>(this->getChildren()))
    {
      if (child == m_fields->m_confirm || !child->isVisible())
        continue;
      auto box = child->boundingBox();
      if (box.size.width >= winSize.width * 0.9f && box.size.height >= winSize.height * 0.9f && !hasContent(child))
        continue;
      child->setVisible(false);
      m_fields->m_hidden.emplace_back(child);
    }
  }

  void closeConfirm()
  {
    auto& f = *m_fields.self();
    auto confirm = f.m_confirm;
    if (!confirm || !confirm->isOpen() || f.m_quitting)
      return;
    confirm->close();
    if (f.m_styled)
    {
      f.m_state = State::Main;
      f.m_column->setShown(true);
      this->fade(f.m_subtitle, true);
    }
    else
    {
      for (auto const& node : f.m_hidden)
        node->setVisible(true);
      f.m_hidden.clear();
    }
    f.m_closedOnFrame = CCDirector::get()->getTotalFrames();
  }

  // What GD's popup does on Exit (PauseLayer::FLAlert_Clicked): onQuit. Once only
  void quitLevel()
  {
    if (m_fields->m_quitting)
      return;
    m_fields->m_quitting = true;
    this->onQuit(nullptr);
  }

  // ! --- Keys --- !

  void keyDown(enumKeyCodes key, double timestamp)
  {
    auto& f = *m_fields.self();
    auto frame = CCDirector::get()->getTotalFrames();
    // GD's own keyDown takes Esc and a controller's B as Quit: here they're back, as keyBackClicked
    auto back = key == KEY_Escape || key == CONTROLLER_B;
    // The confirmation open: every key is its own, none reaches what's hidden (the press that opened it excepted)
    if (f.m_confirm && f.m_confirm->isOpen())
    {
      if (back && f.m_openedOnFrame == frame)
        return;
      f.m_confirm->onKey(key);
      return;
    }
    // This press already went back
    if (back && f.m_closedOnFrame == frame)
      return;
    if (f.m_styled && f.m_state == State::More && back)
    {
      ms::playSound(Theme::get().moreCloseSound);
      return this->showMore(false);
    }
    if (f.m_styled)
    {
      auto column = f.m_state == State::Main ? f.m_column : f.m_state == State::More ? f.m_more : nullptr;
      if (column && column->onKey(key))
        return;
    }
    PauseLayer::keyDown(key, timestamp);
  }

  // Esc: back from the confirmation or More to the pause menu (never on with the level); from it, GD's own (Quit)
  void keyBackClicked()
  {
    auto& f = *m_fields.self();
    auto frame = CCDirector::get()->getTotalFrames();
    if (f.m_confirm && f.m_confirm->isOpen())
    {
      // Not the press that opened it
      if (f.m_openedOnFrame != frame)
        f.m_confirm->keyBack();
      return;
    }
    if (f.m_closedOnFrame == frame)
      return;
    if (f.m_styled && f.m_state == State::More)
    {
      ms::playSound(Theme::get().moreCloseSound);
      return this->showMore(false);
    }
    PauseLayer::keyBackClicked();
  }
};
