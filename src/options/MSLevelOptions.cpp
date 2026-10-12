#include "MSOptionsData.hpp"
#include "MSOptionsScreen.hpp"
#include "MSOptionsText.hpp"

#include "../Theme.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/EditLevelLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>

using namespace geode::prelude;

// A level's own options (its page's options button) as the mod's screen: GD's toggles for that level (low detail,
// no shake), each a click on GD's own button in its options window (made for it, never shown). GD WINDOW opens GD's
class MSLevelOptionsScreen : public MSOptionsScreen
{
public:
  static MSLevelOptionsScreen* create(GJGameLevel* level, std::function<void()> vanilla)
  {
    auto screen = new MSLevelOptionsScreen();
    if (screen->setup(level, std::move(vanilla)))
    {
      screen->autorelease();
      return screen;
    }
    delete screen;
    return nullptr;
  }

protected:
  bool setup(GJGameLevel* level, std::function<void()> vanilla)
  {
    auto en = english();
    if (!level || !this->initOptions(en ? "LEVEL OPTIONS" : "ПАРАМЕТРЫ УРОВНЯ"))
      return false;
    m_layer = this->drive(GameLevelOptionsLayer::create(level));
    if (!m_layer)
      return false;
    if (vanilla)
      this->addBottomButton(en ? "GD WINDOW" : "ОКНО GD", Theme::get().screenButtonWidth,
                            [this, vanilla] { this->close(vanilla); });
    this->addText(std::string(level->m_levelName));
    for (auto const& toggle : ms::options::toggles(static_cast<GJOptionsLayer*>(m_layer.data())))
    {
      if (!toggle.toggler)
        continue;
      MSPauseColumn::Choice choice;
      choice.value = [toggler = toggle.toggler, en] {
        return std::string(toggler->isToggled() ? (en ? "ON" : "ВКЛ") : (en ? "OFF" : "ВЫКЛ"));
      };
      choice.change = [toggler = toggle.toggler](int) { toggler->activate(); };
      m_column->addChoice(ms::options::label(toggle, en), std::move(choice));
      m_column->setInfo(ms::options::info(toggle, en));
    }
    this->finishOptions();
    return true;
  }

  Ref<GameLevelOptionsLayer> m_layer;
};

namespace ms::options
{
  void openLevelOptions(GJGameLevel* level)
  {
    if (auto screen = MSLevelOptionsScreen::create(level, nullptr))
      screen->show();
  }
} // namespace ms::options

namespace
{
  bool vanillaMenu() { return Mod::get()->getSettingValue<bool>("vanilla-menu"); }
} // namespace

class $modify(MSLevelInfoOptions, LevelInfoLayer)
{
  struct Fields
  {
    bool m_vanilla = false;
  };

  void onLevelOptions(CCObject* sender)
  {
    if (vanillaMenu() || m_fields->m_vanilla)
      return LevelInfoLayer::onLevelOptions(sender);
    Ref<LevelInfoLayer> self = this;
    auto screen = MSLevelOptionsScreen::create(m_level, [self] {
      auto layer = static_cast<MSLevelInfoOptions*>(self.data());
      layer->m_fields->m_vanilla = true;
      layer->LevelInfoLayer::onLevelOptions(nullptr);
      layer->m_fields->m_vanilla = false;
    });
    if (screen)
      screen->show();
    else
      LevelInfoLayer::onLevelOptions(sender);
  }
};

class $modify(MSEditLevelOptions, EditLevelLayer)
{
  struct Fields
  {
    bool m_vanilla = false;
  };

  void onLevelOptions(CCObject* sender)
  {
    if (vanillaMenu() || m_fields->m_vanilla)
      return EditLevelLayer::onLevelOptions(sender);
    Ref<EditLevelLayer> self = this;
    auto screen = MSLevelOptionsScreen::create(m_level, [self] {
      auto layer = static_cast<MSEditLevelOptions*>(self.data());
      layer->m_fields->m_vanilla = true;
      layer->EditLevelLayer::onLevelOptions(nullptr);
      layer->m_fields->m_vanilla = false;
    });
    if (screen)
      screen->show();
    else
      EditLevelLayer::onLevelOptions(sender);
  }
};
