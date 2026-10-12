#include "MSAlerts.hpp"
#include "MSOptionsScreen.hpp"
#include "MSOptionsText.hpp"

#include "../Theme.hpp"
#include "../ui/MSStyle.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/AccountLoginLayer.hpp>
#include <Geode/modify/AccountRegisterLayer.hpp>

using namespace geode::prelude;

namespace
{
  // Between a field's caption and its box, and between fields
  constexpr float CAPTION_GAP = 2;
  constexpr float FIELD_GAP = 5;
} // namespace

// GD's account form (log in, register) as a screen of the mod's: GD's own text fields moved into it (what's typed is
// GD's, hidden passwords too), each under its caption, which follows GD's (red when GD says it's wrong); the form's
// buttons as rows. The form itself stays under it, unseen, and does the work; it closing (signed in) closes this too
class MSFormScreen : public MSOptionsScreen
{
public:
  struct Field
  {
    CCTextInputNode* input = nullptr;
    CCLabelBMFont* caption = nullptr;
    char const* en = "";
    char const* ru = "";
  };

  struct Action
  {
    char const* en = "";
    char const* ru = "";
    std::function<void()> run;
  };

  static MSFormScreen* create(std::string const& title, FLAlertLayer* form, std::vector<Field> fields,
                              std::vector<Action> actions, LoadingCircle* loading)
  {
    auto screen = new MSFormScreen();
    if (screen->setup(title, form, std::move(fields), std::move(actions), loading))
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
    if (!m_form->getParent())
    {
      m_formGone = true;
      this->close();
      return;
    }
    // GD's captions as they are now (its errors)
    for (auto& shown : m_captions)
    {
      if (!shown.gd || !shown.label)
        continue;
      std::string text = shown.gd->getString();
      if (text != shown.text)
      {
        shown.text = text;
        shown.label->setString(ms::options::upper(ms::options::plain(text)).c_str());
      }
      shown.label->setColor(ccc3BEqual(shown.gd->getColor(), ccWHITE) ? Theme::get().achievementsTextColor : shown.gd->getColor());
    }
    if (m_status)
      m_status->setVisible(m_loading && m_loading->isVisible());
  }

protected:
  struct Caption
  {
    CCLabelBMFont* gd = nullptr;
    CCLabelBMFont* label = nullptr;
    std::string text;
  };

  bool setup(std::string const& title, FLAlertLayer* form, std::vector<Field> fields, std::vector<Action> actions,
             LoadingCircle* loading)
  {
    auto en = english();
    if (!form || !this->initOptions(title))
      return false;
    auto const& theme = Theme::get();
    m_form = form;
    m_loading = loading;
    for (auto const& field : fields)
    {
      if (!field.input)
        continue;
      // The caption: GD's text while it says something, else the mod's
      if (auto caption = typeinfo_cast<CCLabelBMFont*>(ms::createLabel(en ? field.en : field.ru)))
      {
        caption->setScale(theme.achievementsTextScale);
        caption->setAnchorPoint({0, 1});
        caption->setColor(theme.achievementsTextColor);
        caption->setPosition({theme.screenLeft + 2, m_top + fromTop()});
        m_mainLayer->addChild(caption, 10);
        m_texts.push_back(caption);
        m_top -= caption->getScaledContentSize().height + CAPTION_GAP;
        if (field.caption && field.caption->getString() && *field.caption->getString())
          m_captions.push_back({field.caption, caption, ""});
      }
      // Its box, and GD's field in it
      auto height = theme.pauseRowHeight;
      auto box = ms::createPanel(theme.rowSprite, theme.pauseRowColor, theme.pauseRowOpacity);
      box->setAnchorPoint({0, 1});
      box->ignoreAnchorPointForPosition(false);
      box->setContentSize({theme.optionsRowWidth, height});
      box->setPosition({theme.screenLeft, m_top + fromTop()});
      m_mainLayer->addChild(box, 9);
      m_texts.push_back(box);
      Ref<CCTextInputNode> input = field.input;
      input->removeFromParentAndCleanup(false);
      auto size = input->getContentSize();
      auto scale = std::min(theme.optionsRowWidth / std::max(size.width, 1.f), height / std::max(size.height, 1.f));
      input->setScale(std::min(scale, 1.f));
      input->setAnchorPoint({0.5f, 0.5f});
      input->setPosition({theme.screenLeft + theme.optionsRowWidth / 2, m_top + fromTop() - height / 2});
      m_mainLayer->addChild(input, 11);
      m_inputs.emplace_back(input);
      m_top -= height + FIELD_GAP;
    }
    for (auto const& action : actions)
      m_column->addRow(en ? action.en : action.ru, action.run);
    this->finishOptions();
    // GD's loading circle: a word under the rows
    if (loading)
    {
      if (auto status = ms::createLabel(en ? "Wait..." : "Подождите..."))
      {
        status->setScale(theme.optionsInfoScale);
        status->setAnchorPoint({0, 1});
        status->setPosition({theme.screenLeft + 2, theme.optionsInfoTop + fromTop()});
        ms::setColor(status, theme.optionsInfoColor);
        status->setVisible(false);
        m_mainLayer->addChild(status, 10);
        m_status = status;
      }
    }
    return true;
  }

  // BACK and Esc: GD's form closes as its own close does
  void closed() override
  {
    if (!m_formGone && m_form->getParent())
    {
      if (auto login = typeinfo_cast<AccountLoginLayer*>(m_form.data()))
        login->onClose(nullptr);
      else if (auto reg = typeinfo_cast<AccountRegisterLayer*>(m_form.data()))
        reg->onClose(nullptr);
    }
  }

  // Typing goes to GD's fields: only Esc and the arrow keys for the rows
  bool onKey(enumKeyCodes key) override
  {
    for (auto const& input : m_inputs)
    {
      if (input->m_selected)
        return false;
    }
    return MSOptionsScreen::onKey(key);
  }

  Ref<FLAlertLayer> m_form;
  LoadingCircle* m_loading = nullptr;
  std::vector<Ref<CCTextInputNode>> m_inputs;
  std::vector<Caption> m_captions;
  CCNode* m_status = nullptr;
  bool m_formGone = false;
};

namespace
{
  // Once GD has shown its form (the next frame), the mod's screen comes over it with its fields
  template <class Make>
  void restyleForm(FLAlertLayer* form, Make make)
  {
    if (!ms::options::restyling() || Mod::get()->getSettingValue<bool>("vanilla-menu"))
      return;
    Ref<FLAlertLayer> keep = form;
    Loader::get()->queueInMainThread([keep, make] {
      if (!keep->getParent())
        return;
      if (auto screen = make(keep))
        screen->show();
    });
  }
} // namespace

class $modify(MSAccountLoginLayer, AccountLoginLayer)
{
  bool init(gd::string username)
  {
    if (!AccountLoginLayer::init(username))
      return false;
    restyleForm(this, [](Ref<FLAlertLayer> form) {
      auto login = static_cast<AccountLoginLayer*>(form.data());
      auto en = MSScreen::english();
      std::vector<MSFormScreen::Field> fields = {
        {login->m_usernameInput, login->m_usernameLabel, "USERNAME", "ИМЯ ПОЛЬЗОВАТЕЛЯ"},
        {login->m_passwordInput, login->m_passwordLabel, "PASSWORD", "ПАРОЛЬ"},
      };
      Ref<AccountLoginLayer> keep = login;
      std::vector<MSFormScreen::Action> actions = {
        {"LOG IN", "ВОЙТИ", [keep] { keep->onSubmit(nullptr); }},
        {"FORGOT USERNAME", "ЗАБЫЛИ ИМЯ", [keep] { keep->onForgotUser(nullptr); }},
        {"FORGOT PASSWORD", "ЗАБЫЛИ ПАРОЛЬ", [keep] { keep->onForgotPassword(nullptr); }},
      };
      return MSFormScreen::create(en ? "LOG IN" : "ВХОД", login, std::move(fields), std::move(actions),
                                  login->m_loadingCircle);
    });
    return true;
  }
};

class $modify(MSAccountRegisterLayer, AccountRegisterLayer)
{
  bool init()
  {
    if (!AccountRegisterLayer::init())
      return false;
    restyleForm(this, [](Ref<FLAlertLayer> form) {
      auto reg = static_cast<AccountRegisterLayer*>(form.data());
      auto en = MSScreen::english();
      std::vector<MSFormScreen::Field> fields = {
        {reg->m_usernameField, reg->m_usernameLabel, "USERNAME", "ИМЯ ПОЛЬЗОВАТЕЛЯ"},
        {reg->m_passwordField, reg->m_passwordLabel, "PASSWORD", "ПАРОЛЬ"},
        {reg->m_confirmPasswordField, reg->m_confirmPasswordLabel, "CONFIRM PASSWORD", "ПОВТОРИТЕ ПАРОЛЬ"},
        {reg->m_emailField, reg->m_emailLabel, "EMAIL", "ПОЧТА"},
        {reg->m_verifyEmailField, reg->m_verifyEmailLabel, "CONFIRM EMAIL", "ПОВТОРИТЕ ПОЧТУ"},
      };
      Ref<AccountRegisterLayer> keep = reg;
      std::vector<MSFormScreen::Action> actions = {
        {"REGISTER", "ЗАРЕГИСТРИРОВАТЬСЯ", [keep] { keep->onSubmit(nullptr); }},
      };
      return MSFormScreen::create(en ? "REGISTER" : "РЕГИСТРАЦИЯ", reg, std::move(fields), std::move(actions),
                                  reg->m_loadingCircle);
    });
    return true;
  }
};
