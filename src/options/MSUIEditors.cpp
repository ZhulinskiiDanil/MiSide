#include "MSAlerts.hpp"
#include "MSOptionsData.hpp"
#include "MSOptionsScreen.hpp"
#include "MSOptionsText.hpp"

#include "../Theme.hpp"
#include "../ui/MSStyle.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/UIOptionsLayer.hpp>
#include <Geode/modify/UIPOptionsLayer.hpp>

using namespace geode::prelude;

namespace
{
  // How often the overlay looks at GD's editor for changes (another button chosen on its preview)
  constexpr float WATCH_TIME = 0.25f;
  // A label right before a control, about level with it, is its name
  constexpr float LABEL_RISE = 14;
  // GD's functions (GeometryDash.exe 2.2081): the editors' Reset and Save/Load (their OK is BACK here)
  constexpr uintptr_t UI_RESET = 0x2a30d0;
  constexpr uintptr_t UI_SAVE_LOAD = 0x2a2cb0;
  constexpr uintptr_t UIP_RESET = 0x2a5f20;

  struct Pair
  {
    char const* en;
    char const* ru;
  };

  constexpr Pair NAMES[] = {
    {"Width:", "ШИРИНА"},     {"Height:", "ВЫСОТА"}, {"Scale:", "МАСШТАБ"}, {"Opacity:", "ПРОЗРАЧНОСТЬ"},
    {"Deadzone:", "МЁРТВАЯ ЗОНА"}, {"Radius:", "РАДИУС"}, {"Dual", "ДВА ИГРОКА"}, {"ModeB", "РЕЖИМ B"},
    {"Snap", "ПРИВЯЗКА"},     {"Split", "РАЗДЕЛИТЬ"}, {"JumpL", "ПРЫЖОК СЛЕВА"},
  };

  std::string name(std::string const& gd, bool en)
  {
    if (!en)
    {
      for (auto const& pair : NAMES)
      {
        if (gd == pair.en)
          return pair.ru;
      }
    }
    auto text = gd;
    if (!text.empty() && text.back() == ':')
      text.pop_back();
    return ms::options::upper(text);
  }

  bool shown(CCNode* node, CCNode* top)
  {
    for (; node && node != top; node = node->getParent())
    {
      if (!node->isVisible())
        return false;
    }
    return node == top;
  }

  CCPoint middle(CCNode* node) { return node->convertToWorldSpace(node->getContentSize() / 2); }

  void labels(CCNode* node, std::vector<CCLabelBMFont*>& out)
  {
    if (auto label = typeinfo_cast<CCLabelBMFont*>(node))
    {
      out.push_back(label);
      return;
    }
    for (auto child : CCArrayExt<CCNode*>(node->getChildren()))
      labels(child, out);
  }

  void items(CCNode* node, std::vector<CCMenuItem*>& out)
  {
    if (auto item = typeinfo_cast<CCMenuItem*>(node))
    {
      out.push_back(item);
      return;
    }
    for (auto child : CCArrayExt<CCNode*>(node->getChildren()))
      items(child, out);
  }

  // A node under one of this kind, below the top
  template <class T>
  bool inside(CCNode* node, CCNode* top)
  {
    for (auto parent = node->getParent(); parent && parent != top; parent = parent->getParent())
    {
      if (typeinfo_cast<T*>(parent))
        return true;
    }
    return false;
  }

  // The label nearest left of a control and level with it (or right of a toggle)
  std::string nearLabel(CCNode* control, CCNode* top, bool right)
  {
    std::vector<CCLabelBMFont*> all;
    labels(top, all);
    auto at = middle(control);
    CCLabelBMFont* best = nullptr;
    auto bestDistance = 1e9f;
    for (auto label : all)
    {
      if (!shown(label, top) || label->hasAncestor(control) || inside<CCTextInputNode>(label, top) ||
          inside<CCMenuItem>(label, top))
        continue;
      auto where = middle(label);
      if (std::abs(where.y - at.y) > LABEL_RISE)
        continue;
      auto distance = right ? where.x - at.x : at.x - where.x;
      if (distance < 0 || distance >= bestDistance)
        continue;
      bestDistance = distance;
      best = label;
    }
    return best ? std::string(best->getString()) : "";
  }

  // The label nearest a control any way round (a toggle's name over or under it), close by
  std::string nearestLabel(CCNode* control, CCNode* top)
  {
    constexpr float CLOSE_BY = 40;
    std::vector<CCLabelBMFont*> all;
    labels(top, all);
    auto at = middle(control);
    CCLabelBMFont* best = nullptr;
    auto bestDistance = CLOSE_BY;
    for (auto label : all)
    {
      if (!shown(label, top) || label->hasAncestor(control) || inside<CCTextInputNode>(label, top) ||
          inside<CCMenuItem>(label, top))
        continue;
      auto distance = middle(label).getDistance(at);
      if (distance < bestDistance)
      {
        bestDistance = distance;
        best = label;
      }
    }
    return best ? std::string(best->getString()) : "";
  }

  bool isPreview(CCNode* node, CCNode* preview)
  {
    if (typeinfo_cast<GJUINode*>(node) || (preview && node == preview))
      return true;
    for (auto child : CCArrayExt<CCNode*>(node->getChildren()))
    {
      if (isPreview(child, preview))
        return true;
    }
    return false;
  }

  // GD's controls stay, unseen and untouchable: the overlay's rows work them. The buttons on its preview stay as GD
  // has them (dragged there)
  void hideControls(CCNode* node, CCNode* preview)
  {
    for (auto child : CCArrayExt<CCNode*>(node->getChildren()))
    {
      if (isPreview(child, preview))
      {
        if (!typeinfo_cast<GJUINode*>(child) && child != preview)
          hideControls(child, preview);
        continue;
      }
      std::function<void(CCNode*)> off = [&](CCNode* n) {
        if (auto menu = typeinfo_cast<CCMenu*>(n))
          menu->setEnabled(false);
        if (auto input = typeinfo_cast<CCTextInputNode*>(n))
          input->setTouchEnabled(false);
        ms::setOpacity(n, 0);
        for (auto c : CCArrayExt<CCNode*>(n->getChildren()))
          off(c);
      };
      off(child);
    }
  }
} // namespace

// GD's platformer or practice UI editor with the mod's column over it: GD's buttons on the preview are dragged as in
// GD; its controls (the chosen button's sizes, opacity, toggles; reset, save and load) are the column's rows, made
// again whenever another button is chosen. HIDE folds the column away to reach what's under it
class MSUIEditorOverlay : public MSOptionsScreen
{
public:
  static MSUIEditorOverlay* create(SetupTriggerPopup* editor, CCNode* preview, bool practice)
  {
    auto screen = new MSUIEditorOverlay();
    if (screen->setup(editor, preview, practice))
    {
      screen->autorelease();
      return screen;
    }
    delete screen;
    return nullptr;
  }

  void update(float dt) override
  {
    MSOptionsScreen::update(dt);
    if (this->isClosing())
      return;
    // GD's editor gone some other way: the overlay too
    if (!m_editor->getParent())
    {
      m_closedByGD = true;
      this->close();
      return;
    }
    m_watch += dt;
    if (m_watch < WATCH_TIME)
      return;
    m_watch = 0;
    if (this->describe() != m_shown)
    {
      this->resetOptions();
      this->build();
    }
  }

protected:
  bool setup(SetupTriggerPopup* editor, CCNode* preview, bool practice)
  {
    auto en = english();
    if (!editor || !this->initOptions(practice ? (en ? "PRACTICE UI" : "ИНТЕРФЕЙС ПРАКТИКИ")
                                               : (en ? "PLATFORMER UI" : "ИНТЕРФЕЙС ПЛАТФОРМЕРА"),
                                      true))
      return false;
    m_editor = editor;
    m_preview = preview;
    m_practice = practice;
    hideControls(editor->m_mainLayer ? editor->m_mainLayer : static_cast<CCNode*>(editor), preview);
    // Back to the column when it's folded
    if (auto unfold = this->addBottomButton(en ? "MENU" : "МЕНЮ", Theme::get().screenButtonWidth,
                                            [this] { this->fold(false); }))
    {
      unfold->setVisible(false);
      m_unfold = unfold;
    }
    this->build();
    return true;
  }

  CCNode* top() { return m_editor->m_mainLayer ? m_editor->m_mainLayer : static_cast<CCNode*>(m_editor.data()); }

  struct Control
  {
    int tag = 0;
    CCNode* node = nullptr;
  };

  // GD's visible controls by their tags: the inputs (with their sliders) and the toggles
  void controls(std::vector<Control>& values, std::vector<Control>& toggles)
  {
    auto top = this->top();
    if (auto inputs = m_editor->m_inputNodes)
    {
      for (auto [tag, object] : CCDictionaryExt<int, CCObject>(inputs))
      {
        auto node = typeinfo_cast<CCNode*>(object);
        if (node && shown(node, top))
          values.push_back({tag, node});
      }
    }
    if (auto all = m_editor->m_valueToggles)
    {
      for (auto [tag, object] : CCDictionaryExt<int, CCObject>(all))
      {
        CCNode* node = typeinfo_cast<CCMenuItemToggler*>(object);
        if (!node)
        {
          if (auto array = typeinfo_cast<CCArray*>(object))
          {
            for (auto entry : CCArrayExt<CCObject*>(array))
            {
              if ((node = typeinfo_cast<CCMenuItemToggler*>(entry)))
                break;
            }
          }
        }
        if (node && shown(node, top))
          toggles.push_back({tag, node});
      }
    }
    auto byPlace = [](Control const& a, Control const& b) {
      auto pa = middle(a.node);
      auto pb = middle(b.node);
      if (std::abs(pa.y - pb.y) > 2)
        return pa.y > pb.y;
      return pa.x < pb.x;
    };
    std::sort(values.begin(), values.end(), byPlace);
    std::sort(toggles.begin(), toggles.end(), byPlace);
  }

  std::string describe()
  {
    std::vector<Control> values;
    std::vector<Control> toggles;
    this->controls(values, toggles);
    std::string out;
    for (auto const& control : values)
      out += fmt::format("v{}|", control.tag);
    for (auto const& control : toggles)
      out += fmt::format("t{}|", control.tag);
    if (auto ui = typeinfo_cast<UIOptionsLayer*>(m_editor.data()); ui && ui->m_nameLabel)
      out += ui->m_nameLabel->getString();
    return out;
  }

  void build()
  {
    auto en = english();
    m_shown = this->describe();
    auto top = this->top();
    // The button chosen on the preview
    if (auto ui = typeinfo_cast<UIOptionsLayer*>(m_editor.data()); ui && ui->m_nameLabel && shown(ui->m_nameLabel, top))
      this->addText(ms::options::plain(ui->m_nameLabel->getString()));

    std::vector<Control> values;
    std::vector<Control> toggles;
    this->controls(values, toggles);
    for (auto const& control : values)
    {
      auto tag = control.tag;
      Ref<SetupTriggerPopup> editor = m_editor;
      auto min = editor->getMinSliderValue(tag);
      auto max = editor->getMaxSliderValue(tag);
      // GD's own name for it, else the label beside it
      std::string gd;
      if (auto names = editor->m_inputLabels)
      {
        if (auto label = typeinfo_cast<CCLabelBMFont*>(names->objectForKey(tag)))
          gd = label->getString();
      }
      if (gd.empty())
        gd = nearLabel(control.node, top, false);
      auto text = name(gd, en);
      if (text.empty())
        text = en ? "VALUE" : "ЗНАЧЕНИЕ";
      if (ms::options::selfTesting())
        log::info("SELFTEST UI value {} named \"{}\" ({} to {})", tag, gd, min, max);
      if (max > min)
      {
        MSPauseColumn::Slider slider;
        slider.get = [editor, tag, min, max] { return (editor->getValue(tag) - min) / (max - min); };
        slider.set = [editor, tag, min, max](float value) { editor->updateValue(tag, min + value * (max - min)); };
        slider.format = [editor, tag](float) {
          auto value = editor->getValue(tag);
          return std::abs(value - std::round(value)) < 0.005f ? fmt::format("{}", static_cast<int>(std::round(value)))
                                                               : fmt::format("{:.2f}", value);
        };
        m_column->addSlider(text, std::move(slider));
      }
      else
      {
        MSPauseColumn::Field field;
        field.get = [editor, tag] { return fmt::format("{}", editor->getValue(tag)); };
        field.set = [editor, tag](std::string const& typed) {
          editor->updateValue(tag, static_cast<float>(std::atof(typed.c_str())));
        };
        field.allowed = "0123456789.-";
        m_column->addField(text, std::move(field));
      }
    }
    for (auto const& control : toggles)
    {
      Ref<CCMenuItemToggler> toggler = static_cast<CCMenuItemToggler*>(control.node);
      auto gdText = nearLabel(toggler, top, true);
      if (gdText.empty())
        gdText = nearLabel(toggler, top, false);
      if (gdText.empty())
        gdText = nearestLabel(toggler, top);
      auto text = name(gdText, en);
      if (ms::options::selfTesting())
        log::info("SELFTEST UI toggle {} named \"{}\"", control.tag, gdText);
      MSPauseColumn::Choice choice;
      choice.value = [toggler, en] {
        return std::string(toggler->isToggled() ? (en ? "ON" : "ВКЛ") : (en ? "OFF" : "ВЫКЛ"));
      };
      choice.change = [toggler](int) { toggler->activate(); };
      m_column->addChoice(text.empty() ? (en ? "OPTION" : "ПАРАМЕТР") : text, std::move(choice));
    }

    // GD's buttons: reset, save / load (its own popup); its OK is BACK here
    std::vector<CCMenuItem*> buttons;
    items(top, buttons);
    for (auto item : buttons)
    {
      if (typeinfo_cast<CCMenuItemToggler*>(item) || !shown(item, top))
        continue;
      Ref<CCMenuItem> button = item;
      if (ms::options::runs(item, UI_RESET) || ms::options::runs(item, UIP_RESET))
      {
        m_column->addRow(en ? "RESET" : "СБРОСИТЬ", [this, button] { this->press(m_editor, [button] { button->activate(); }); });
        m_column->setInfo(en ? "Puts the buttons back where GD has them." : "Возвращает кнопки на места по умолчанию.");
      }
      else if (ms::options::runs(item, UI_SAVE_LOAD))
      {
        m_column->addRow(en ? "SAVE / LOAD" : "СОХРАНИТЬ / ЗАГРУЗИТЬ",
                         [this, button] { this->press(m_editor, [button] { button->activate(); }); });
        m_column->setInfo(en ? "GD's slots for layouts." : "Слоты GD для раскладок.");
      }
    }
    m_column->addRow(en ? "HIDE MENU" : "СКРЫТЬ МЕНЮ", [this] { this->fold(true); });
    m_column->setInfo(en ? "Folds this column away to drag the buttons under it."
                         : "Убирает колонку, чтобы перетащить кнопки под ней.");
    this->finishOptions();
  }

  // The column and its shade out of the way, MENU in the bottom row to bring them back
  void fold(bool folded)
  {
    m_column->setShown(!folded);
    for (auto node : m_texts)
      node->setVisible(!folded);
    if (m_shade)
      m_shade->setVisible(!folded);
    if (m_unfold)
      m_unfold->setVisible(folded);
  }

  // BACK and Esc: GD's editor closes as its OK closes it (the layout saved)
  void closed() override
  {
    if (!m_closedByGD && m_editor && m_editor->getParent())
      m_editor->onClose(nullptr);
  }

  Ref<SetupTriggerPopup> m_editor;
  CCNode* m_preview = nullptr;
  bool m_practice = false;
  bool m_closedByGD = false;
  CCNode* m_unfold = nullptr;
  std::string m_shown;
  float m_watch = 0;
};

namespace
{
  // Once GD has shown its editor (the next frame), the overlay comes over it
  void overlay(SetupTriggerPopup* editor, CCNode* preview, bool practice)
  {
    if (Mod::get()->getSettingValue<bool>("vanilla-menu"))
      return;
    Ref<SetupTriggerPopup> keep = editor;
    Loader::get()->queueInMainThread([keep, preview, practice] {
      if (!keep->getParent())
        return;
      if (auto screen = MSUIEditorOverlay::create(keep, preview, practice))
        screen->show();
    });
  }
} // namespace

class $modify(MSUIOptionsLayer, UIOptionsLayer)
{
  bool init(bool dual)
  {
    if (!UIOptionsLayer::init(dual))
      return false;
    overlay(this, nullptr, false);
    return true;
  }
};

class $modify(MSUIPOptionsLayer, UIPOptionsLayer)
{
  bool init()
  {
    if (!UIPOptionsLayer::init())
      return false;
    overlay(this, m_practiceNode, true);
    return true;
  }
};
