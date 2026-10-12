#include "MSAlerts.hpp"

#include "MSOptionsScreen.hpp"
#include "MSOptionsText.hpp"
#include "MSWindows.hpp"

#include <Geode/modify/DialogLayer.hpp>
#include <Geode/modify/FLAlertLayer.hpp>

using namespace geode::prelude;

namespace
{
  struct Pair
  {
    char const* en;
    char const* ru;
  };

  // GD's usual buttons
  constexpr Pair BUTTONS[] = {
    {"OK", "ОК"},     {"Ok", "ОК"},         {"Cancel", "ОТМЕНА"}, {"Yes", "ДА"},   {"No", "НЕТ"},
    {"Close", "ЗАКРЫТЬ"}, {"Back", "НАЗАД"}, {"Exit", "ВЫЙТИ"},    {"Delete", "УДАЛИТЬ"}, {"Next", "ДАЛЕЕ"},
  };

  std::string button(std::string const& text, bool en)
  {
    if (!en)
    {
      for (auto const& pair : BUTTONS)
      {
        if (text == pair.en)
          return pair.ru;
      }
    }
    return ms::options::upper(ms::options::plain(text));
  }
} // namespace

// GD's popup or a dialog's line as a screen of the mod's: its title, its text, its buttons as rows. A button closes it
// and then answers as GD's would; Esc and BACK answer as GD's Esc
class MSMessage : public MSOptionsScreen
{
public:
  static MSMessage* create(std::string const& title, std::string const& text, std::vector<std::string> const& buttons,
                           std::function<void(int)> chosen, std::function<void()> back)
  {
    auto screen = new MSMessage();
    if (screen->setup(title, text, buttons, std::move(chosen), std::move(back)))
    {
      screen->autorelease();
      return screen;
    }
    delete screen;
    return nullptr;
  }

protected:
  bool setup(std::string const& title, std::string const& text, std::vector<std::string> const& buttons,
             std::function<void(int)> chosen, std::function<void()> back)
  {
    if (!this->initOptions(ms::options::upper(title)))
      return false;
    m_chosen = std::move(chosen);
    m_back = std::move(back);
    this->addText(text);
    for (int i = 0; i < static_cast<int>(buttons.size()); i++)
    {
      m_column->addRow(buttons[i], [this, i] {
        if (m_answer != NONE)
          return;
        m_answer = i;
        this->close();
      });
    }
    this->finishOptions();
    return true;
  }

  // Once it's gone: the button's answer, or Esc's
  void closed() override
  {
    auto answer = m_answer;
    auto chosen = m_chosen;
    auto back = m_back;
    if (answer >= 0)
    {
      if (chosen)
        chosen(answer);
    }
    else if (back)
      back();
  }

  static constexpr int NONE = -1;
  int m_answer = NONE;
  std::function<void(int)> m_chosen;
  std::function<void()> m_back;
};

class $modify(MSAlertLayer, FLAlertLayer)
{
  struct Fields
  {
    // Made with its texts, and what init put in it then
    bool m_texts = false;
    std::string m_title;
    std::string m_desc;
    std::string m_button1;
    std::string m_button2;
    unsigned m_mainCount = 0;
    unsigned m_menuCount = 0;
  };

  bool init(FLAlertLayerProtocol* delegate, char const* title, gd::string desc, char const* btn1, char const* btn2,
            float width, bool scroll, float height, float textScale)
  {
    std::string text = desc;
    if (!FLAlertLayer::init(delegate, title, desc, btn1, btn2, width, scroll, height, textScale))
      return false;
    auto& f = *m_fields.self();
    f.m_texts = true;
    f.m_title = title ? title : "";
    f.m_desc = text;
    f.m_button1 = btn1 ? btn1 : "";
    f.m_button2 = btn2 ? btn2 : "";
    f.m_mainCount = m_mainLayer ? m_mainLayer->getChildrenCount() : 0;
    f.m_menuCount = m_buttonMenu ? m_buttonMenu->getChildrenCount() : 0;
    return true;
  }

  void show()
  {
    if (ms::options::restyling() && ms::options::showAlert(this))
      return;
    FLAlertLayer::show();
    // A request's progress, the UI's save slots: shown, and the mod's screen over them
    if (ms::options::restyling() && (typeinfo_cast<UploadActionPopup*>(this) || typeinfo_cast<UISaveLoadLayer*>(this)))
      ms::options::mirrorPopup(this);
  }

  // Only GD's plain popup, as init made it: nothing added since (a toggle, an input), nothing of a subclass's
  bool restylable()
  {
    auto& f = *m_fields.self();
    if (!f.m_texts || typeid(*this) != typeid(FLAlertLayer))
      return false;
    auto main = m_mainLayer ? m_mainLayer->getChildrenCount() : 0;
    auto menu = m_buttonMenu ? m_buttonMenu->getChildrenCount() : 0;
    return main == f.m_mainCount && menu == f.m_menuCount && !f.m_button1.empty();
  }
};

class $modify(MSDialogLayer, DialogLayer)
{
  void addToMainScene()
  {
    if (ms::options::restyling())
      return ms::options::showDialog(this);
    DialogLayer::addToMainScene();
  }
};

namespace ms::options
{
  std::string plain(std::string const& text)
  {
    std::string out;
    out.reserve(text.size());
    for (size_t i = 0; i < text.size(); i++)
    {
      if (text[i] == '<')
      {
        auto end = text.find('>', i);
        if (end != std::string::npos && end - i <= 6)
        {
          auto tag = text.substr(i + 1, end - i - 1);
          auto start = tag.empty() || tag[0] != '/' ? 0 : 1;
          auto letters = start;
          while (letters < tag.size() && std::isalpha(static_cast<unsigned char>(tag[letters])))
            letters++;
          auto digits = letters;
          while (digits < tag.size() && std::isdigit(static_cast<unsigned char>(tag[digits])))
            digits++;
          if (letters > start && letters - start <= 2 && digits == tag.size())
          {
            i = end;
            continue;
          }
        }
      }
      out.push_back(text[i]);
    }
    return out;
  }

  bool showAlert(FLAlertLayer* alert)
  {
    auto hooked = static_cast<MSAlertLayer*>(alert);
    if (!alert || !hooked->restylable())
      return false;
    auto const& f = *hooked->m_fields.self();
    auto en = MSScreen::english();
    std::vector<std::string> buttons{button(f.m_button1, en)};
    if (!f.m_button2.empty())
      buttons.push_back(button(f.m_button2, en));
    Ref<FLAlertLayer> keep = alert;
    auto screen = MSMessage::create(
      plain(f.m_title), plain(f.m_desc), buttons,
      [keep](int answer) {
        // What GD's buttons do: the delegate told, the popup gone
        if (answer == 0)
          keep->onBtn1(nullptr);
        else
          keep->onBtn2(nullptr);
      },
      [keep] { keep->keyBackClicked(); });
    if (!screen)
      return false;
    screen->show();
    return true;
  }

  namespace
  {
    // One line of the dialog after another, then its delegate told it closed, as GD's dialog does
    void dialogLine(Ref<DialogLayer> dialog, unsigned index)
    {
      auto objects = dialog->m_dialogObjects;
      auto count = objects ? objects->count() : 0u;
      if (index >= count)
      {
        if (dialog->m_delegate)
          dialog->m_delegate->dialogClosed(dialog);
        return;
      }
      auto line = typeinfo_cast<DialogObject*>(objects->objectAtIndex(index));
      if (!line)
        return dialogLine(dialog, index + 1);
      auto en = MSScreen::english();
      auto last = index + 1 >= count;
      std::string next = last ? (en ? "OK" : "ОК") : (en ? "NEXT" : "ДАЛЕЕ");
      auto screen = MSMessage::create(
        plain(line->m_character), plain(line->m_text), {next},
        [dialog, index](int) { dialogLine(dialog, index + 1); },
        [dialog, index] { dialogLine(dialog, index + 1); });
      if (screen)
        screen->show();
    }
  } // namespace

  void showDialog(DialogLayer* dialog)
  {
    if (!dialog)
      return;
    dialogLine(dialog, 0);
  }

  void adoptPopup(CCNode* node, int z)
  {
    if (!node)
      return;
    if (auto dialog = typeinfo_cast<DialogLayer*>(node))
      return showDialog(dialog);
    if (auto alert = typeinfo_cast<FLAlertLayer*>(node); alert && showAlert(alert))
      return;
    auto scene = CCDirector::get()->getRunningScene();
    if (!scene)
      return;
    scene->addChild(node, std::max(scene->getHighestChildZ(), z));
  }
} // namespace ms::options
