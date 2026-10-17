#include "MSOptionsData.hpp"
#include "MSOptionsScreen.hpp"
#include "MSOptionsText.hpp"

#include "../Theme.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>

using namespace geode::prelude;

// The editor's options (its pause's Options) as a screen of the mod's: GD's toggles, each a click on GD's own button
// in its options window (made for it, never shown), and the build tab's buttons per row and rows, applied as GD's
// window applies them when it closes. GD WINDOW opens GD's own window (this screen's changes applied first)
class MSEditorOptionsScreen : public MSOptionsScreen
{
public:
  static MSEditorOptionsScreen* create()
  {
    auto screen = new MSEditorOptionsScreen();
    if (screen->setup())
    {
      screen->autorelease();
      return screen;
    }
    delete screen;
    return nullptr;
  }

protected:
  bool setup()
  {
    auto en = english();
    if (!this->initOptions(en ? "EDITOR OPTIONS" : "НАСТРОЙКИ РЕДАКТОРА"))
      return false;
#if GEODE_COMP_GD_VERSION >= 22090
    return false; // GD 2.209 removed or rebuilt this vanilla screen
#endif
    m_layer = this->drive(EditorOptionsLayer::create());
    if (!m_layer)
      return false;
    this->addBottomButton(en ? "GD WINDOW" : "ОКНО GD", Theme::get().screenButtonWidth, [this] {
      this->close([] {
#if GEODE_COMP_GD_VERSION >= 22090
        return; // the vanilla window is gone in GD 2.209
#endif
        if (auto layer = EditorOptionsLayer::create())
          layer->show();
      });
    });

    m_column->addHeader(en ? "BUILD TAB" : "ПАНЕЛЬ ОБЪЕКТОВ");
    this->addCount(en ? "BUTTONS PER ROW" : "КНОПОК В РЯДУ", true);
    this->addCount(en ? "BUTTON ROWS" : "РЯДОВ КНОПОК", false);

    m_column->addHeader(en ? "OPTIONS" : "ПАРАМЕТРЫ");
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

  // GD's arrows: their tag says the way (1 up, else down), its window keeps the number in range
  void addCount(std::string const& text, bool perRow)
  {
    MSPauseColumn::Choice choice;
    choice.value = [this, perRow] { return std::to_string(perRow ? m_layer->m_buttonsPerRow : m_layer->m_buttonRows); };
    choice.change = [this, perRow](int direction) {
      auto arrow = CCNode::create();
      arrow->setTag(direction > 0 ? 1 : 0);
      if (perRow)
        m_layer->onButtonsPerRow(arrow);
      else
        m_layer->onButtonRows(arrow);
    };
    m_column->addChoice(text, std::move(choice));
    m_column->setInfo(english() ? "How the editor's build tab lays its buttons out; Left/Right change it."
                                : "Как панель объектов редактора раскладывает кнопки; влево/вправо — изменить.");
  }

  // What GD's window does when it closes: the counts saved and the editor's tab laid out again
  void apply()
  {
    if (m_applied || !m_layer)
      return;
    m_applied = true;
    m_layer->onClose(nullptr);
  }

  void closed() override { this->apply(); }

  // GD WINDOW: this screen's changes go in before GD's own window opens
  void onExit() override
  {
    this->apply();
    MSOptionsScreen::onExit();
  }

  Ref<EditorOptionsLayer> m_layer;
  bool m_applied = false;
};

class $modify(MSEditorPauseLayer, EditorPauseLayer)
{
  void onOptions(CCObject* sender)
  {
    if (Mod::get()->getSettingValue<bool>("vanilla-menu"))
      return EditorPauseLayer::onOptions(sender);
    if (auto screen = MSEditorOptionsScreen::create())
      screen->show();
    else
      EditorPauseLayer::onOptions(sender);
  }
};
