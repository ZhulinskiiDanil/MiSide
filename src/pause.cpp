#include "ui/MSConfirm.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>

using namespace geode::prelude;

namespace
{
  // Over everything of the pause menu
  constexpr int CONFIRM_Z = 1000;
  // GD's exit confirmation option: PauseLayer::tryQuit asks only when it's on
  constexpr char const* CONFIRM_EXIT = "0167";
  // A node this much of the screen or more, with no button or text in it, is the pause's background (its darkening,
  // a blur another mod adds): it stays while the confirmation is open
  constexpr float BACKGROUND_SHARE = 0.9f;

  // Buttons or text somewhere in it: a menu as big as the screen (cocos makes them so) isn't a background
  bool hasContent(CCNode* node)
  {
    if (typeinfo_cast<CCMenu*>(node) || typeinfo_cast<CCMenuItem*>(node) || typeinfo_cast<CCLabelProtocol*>(node))
      return true;
    for (auto child : CCArrayExt<CCNode*>(node->getChildren()))
    {
      if (hasContent(child))
        return true;
    }
    return false;
  }
} // namespace

// The same confirmation as the main menu's, in place of GD's "Exit Level" popup: the pause menu's own nodes hide
// while it's open, its background (the darkened, maybe blurred level) stays. No Mita, no logo. Yes leaves the level
// as the popup's Exit does; No and Esc go back to the pause menu, the level still paused
class $modify(MSPauseLayer, PauseLayer)
{
  struct Fields
  {
    MSConfirm* m_confirm = nullptr;
    // What was shown before it opened, to show again
    std::vector<Ref<CCNode>> m_hidden;
    unsigned m_closedOnFrame = 0;
    bool m_quitting = false;
  };

  // Made with the pause menu, hidden: its touches are taken the way the pause menu's own buttons' are (a menu added to
  // the blocking layer later may get none)
  void customSetup()
  {
    PauseLayer::customSetup();
    if (Mod::get()->getSettingValue<bool>("vanilla-menu"))
      return;
    auto english = Mod::get()->getSettingValue<std::string>("menu-language") == "EN";
    MSConfirm::Texts texts{english ? "QUIT THE LEVEL?" : "ВЫЙТИ ИЗ УРОВНЯ?", english ? "YES" : "ДА",
                           english ? "NO" : "НЕТ"};
    auto confirm = MSConfirm::create(texts, [this] { this->quitLevel(); }, [this] { this->closeConfirm(); });
    if (!confirm)
      return;
    confirm->setID("quit-confirm"_spr);
    this->addChild(confirm, CONFIRM_Z);
    m_fields->m_confirm = confirm;
  }

  void tryQuit(CCObject* sender)
  {
    // GD asks only with its option on; the mod's setting asks always. Neither: it leaves at once, as it always did
    auto ask = Mod::get()->getSettingValue<bool>("confirm-level-exit") ||
               GameManager::get()->getGameVariable(CONFIRM_EXIT);
    if (!ask || !this->openConfirm())
      return PauseLayer::tryQuit(sender);
  }

  void keyDown(enumKeyCodes key, double timestamp)
  {
    // Open, every key is the confirmation's: none reaches the hidden pause menu
    if (auto confirm = m_fields->m_confirm; confirm && confirm->isOpen())
    {
      confirm->onKey(key);
      return;
    }
    PauseLayer::keyDown(key, timestamp);
  }

  // Esc: back to the pause menu, not on with the level
  void keyBackClicked()
  {
    if (auto confirm = m_fields->m_confirm; confirm && confirm->isOpen())
      return confirm->keyBack();
    if (m_fields->m_closedOnFrame == CCDirector::get()->getTotalFrames())
      return;
    PauseLayer::keyBackClicked();
  }

  bool openConfirm()
  {
    if (m_fields->m_quitting)
      return true;
    // None (its text couldn't be made): GD's popup
    auto confirm = m_fields->m_confirm;
    if (!confirm)
      return false;
    if (confirm->isOpen())
      return true;

    // The pause menu's own nodes (its buttons, sliders, labels, progress bars, and what mods added there) hide and
    // take nothing; the background stays
    auto winSize = CCDirector::get()->getWinSize();
    m_fields->m_hidden.clear();
    for (auto child : CCArrayExt<CCNode*>(this->getChildren()))
    {
      if (child == confirm || !child->isVisible())
        continue;
      auto box = child->boundingBox();
      if (box.size.width >= winSize.width * BACKGROUND_SHARE &&
          box.size.height >= winSize.height * BACKGROUND_SHARE && !hasContent(child))
        continue;
      child->setVisible(false);
      m_fields->m_hidden.emplace_back(child);
    }
    confirm->open();
    return true;
  }

  void closeConfirm()
  {
    auto confirm = m_fields->m_confirm;
    if (!confirm || !confirm->isOpen() || m_fields->m_quitting)
      return;
    confirm->close();
    for (auto const& node : m_fields->m_hidden)
      node->setVisible(true);
    m_fields->m_hidden.clear();
    m_fields->m_closedOnFrame = CCDirector::get()->getTotalFrames();
  }

  // What GD's popup does on Exit (PauseLayer::FLAlert_Clicked): onQuit. Once only
  void quitLevel()
  {
    if (m_fields->m_quitting)
      return;
    m_fields->m_quitting = true;
    this->onQuit(nullptr);
  }
};
