#include "MSAlerts.hpp"
#include "MSOptionsData.hpp"
#include "MSOptionsScreen.hpp"
#include "MSOptionsText.hpp"

#include "../Theme.hpp"
#include "../ui/MSStyle.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/AccountLoginLayer.hpp>
#include <Geode/modify/AccountRegisterLayer.hpp>
#include <Geode/modify/GJAccountSettingsLayer.hpp>

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
    std::string en;
    std::string ru;
  };

  struct Action
  {
    std::string en;
    std::string ru;
    std::function<void()> run;
    std::string info;
  };

  struct Option
  {
    std::string text;
    MSPauseColumn::Choice choice;
  };

  static MSFormScreen* create(std::string const& title, FLAlertLayer* form, std::vector<Field> fields,
                              std::vector<Action> actions, LoadingCircle* loading, std::vector<Option> options = {})
  {
    auto screen = new MSFormScreen();
    if (screen->setup(title, form, std::move(fields), std::move(actions), loading, std::move(options)))
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
             LoadingCircle* loading, std::vector<Option> options)
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
      if (auto caption = typeinfo_cast<CCLabelBMFont*>(ms::createLabel(en ? field.en : field.ru, theme.font)))
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
    for (auto& option : options)
      m_column->addChoice(option.text, std::move(option.choice));
    for (auto const& action : actions)
    {
      m_column->addRow(en ? action.en : action.ru, action.run);
      m_column->setInfo(action.info);
    }
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
      else if (auto settings = typeinfo_cast<GJAccountSettingsLayer*>(m_form.data()))
        settings->onClose(nullptr);
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

namespace
{
  // GD's profile settings' words
  struct Pair
  {
    char const* en;
    char const* ru;
  };

  constexpr Pair PROFILE_WORDS[] = {
    {"All", "ВСЕ"},           {"Friends", "ДРУЗЬЯ"}, {"None", "НИКТО"}, {"Me", "ТОЛЬКО Я"},
    {"Off", "ВЫКЛ"},          {"YouTube:", "YOUTUBE"}, {"Twitter:", "TWITTER"}, {"Twitch:", "TWITCH"},
    {"Discord:", "DISCORD"},  {"Instagram:", "INSTAGRAM"}, {"TikTok:", "TIKTOK"}, {"Custom:", "СВОЁ"},
  };

  std::string profileWord(std::string const& gd, bool en)
  {
    if (!en)
    {
      for (auto const& pair : PROFILE_WORDS)
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

  CCPoint middleOf(CCNode* node) { return node->convertToWorldSpace(node->getContentSize() / 2); }

  void allLabels(CCNode* node, std::vector<CCLabelBMFont*>& out)
  {
    if (!node->isVisible())
      return;
    if (auto label = typeinfo_cast<CCLabelBMFont*>(node))
    {
      out.push_back(label);
      return;
    }
    for (auto child : CCArrayExt<CCNode*>(node->getChildren()))
      allLabels(child, out);
  }

  // The label nearest a node on one side, about level with it
  CCLabelBMFont* labelBeside(CCNode* node, CCNode* top, bool right, float rise = 14)
  {
    std::vector<CCLabelBMFont*> labels;
    allLabels(top, labels);
    auto at = middleOf(node);
    CCLabelBMFont* best = nullptr;
    auto bestDistance = 1e9f;
    for (auto label : labels)
    {
      if (label->hasAncestor(node))
        continue;
      auto where = middleOf(label);
      if (std::abs(where.y - at.y) > rise)
        continue;
      auto distance = right ? where.x - at.x : at.x - where.x;
      if (distance < 0 || distance >= bestDistance)
        continue;
      bestDistance = distance;
      best = label;
    }
    return best;
  }

  // A group of GD's radio toggles as one choice: the one on is the value, Left/Right turn on the next
  MSFormScreen::Option profileChoice(std::string const& text, CCArray* array, CCNode* top, bool en)
  {
    std::vector<Ref<CCMenuItemToggler>> togglers;
    std::vector<std::string> names;
    if (array)
    {
      for (auto object : CCArrayExt<CCObject*>(array))
      {
        if (auto toggler = typeinfo_cast<CCMenuItemToggler*>(object))
        {
          togglers.emplace_back(toggler);
          auto label = labelBeside(toggler, top, true);
          names.push_back(label ? profileWord(label->getString(), en) : std::to_string(names.size() + 1));
        }
      }
    }
    MSFormScreen::Option option;
    option.text = text;
    option.choice.value = [togglers, names] {
      for (size_t i = 0; i < togglers.size(); i++)
      {
        if (togglers[i]->isToggled())
          return names[i];
      }
      return std::string("-");
    };
    option.choice.change = [togglers](int direction) {
      auto count = static_cast<int>(togglers.size());
      if (count == 0)
        return;
      auto now = 0;
      for (int i = 0; i < count; i++)
      {
        if (togglers[i]->isToggled())
          now = i;
      }
      auto next = ((now + direction) % count + count) % count;
      togglers[next]->activate();
    };
    return option;
  }
} // namespace

// GD's profile settings (from the profile page) as the mod's screen: who may message, send friend requests and see
// the comment history (GD's own choices), the links as GD's own fields with their help, and UPDATE as GD's
class $modify(MSAccountSettingsLayer, GJAccountSettingsLayer)
{
  bool init(int accountID)
  {
    if (!GJAccountSettingsLayer::init(accountID))
      return false;
    if (Mod::get()->getSettingValue<bool>("vanilla-menu"))
      return true;
    Ref<GJAccountSettingsLayer> keep = this;
    Loader::get()->queueInMainThread([keep] {
      if (!keep->getParent())
        return;
      auto en = MSScreen::english();
      auto top = keep->m_mainLayer ? keep->m_mainLayer : static_cast<CCNode*>(keep.data());
      std::vector<MSFormScreen::Option> options;
      options.push_back(profileChoice(en ? "MESSAGES FROM" : "СООБЩЕНИЯ ОТ", keep->m_messageSettings, top, en));
      options.push_back(profileChoice(en ? "FRIEND REQUESTS FROM" : "ЗАЯВКИ В ДРУЗЬЯ ОТ", keep->m_friendRequestSettings,
                                      top, en));
      options.push_back(profileChoice(en ? "COMMENT HISTORY TO" : "ИСТОРИЯ КОММЕНТАРИЕВ ДЛЯ", keep->m_commentSettings,
                                      top, en));
      std::vector<MSFormScreen::Field> fields;
      std::vector<MSFormScreen::Action> actions;
      if (auto inputs = keep->m_inputs)
      {
        for (auto object : CCArrayExt<CCObject*>(inputs))
        {
          auto input = typeinfo_cast<CCTextInputNode*>(object);
          if (!input)
            continue;
          auto caption = labelBeside(input, top, false, 20);
          std::string gd = caption ? caption->getString() : "";
          fields.push_back({input, nullptr, profileWord(gd, true), profileWord(gd, false)});
        }
      }
      actions.push_back({"UPDATE", "ОБНОВИТЬ", [keep] { keep->onUpdate(nullptr); },
                         en ? "Saves these settings to your account." : "Сохраняет эти настройки в аккаунте."});
      // Each link's help (GD's info buttons): the button right of its field
      std::vector<CCMenuItem*> items;
      std::function<void(CCNode*)> collect = [&](CCNode* node) {
        if (auto item = typeinfo_cast<CCMenuItem*>(node))
        {
          items.push_back(item);
          return;
        }
        for (auto child : CCArrayExt<CCNode*>(node->getChildren()))
          collect(child);
      };
      collect(top);
      for (auto item : items)
      {
        if (typeinfo_cast<CCMenuItemToggler*>(item) || !item->isVisible())
          continue;
        auto caption = labelBeside(item, top, false, 20);
        if (!caption || std::string(caption->getString()).find(':') == std::string::npos)
          continue;
        Ref<CCMenuItem> button = item;
        auto word = profileWord(caption->getString(), en);
        // Two by one caption: numbered
        auto same = std::count_if(actions.begin(), actions.end(), [&](auto const& a) { return a.ru.starts_with("СПРАВКА: " + word); });
        if (same > 0)
          word += fmt::format(" ({})", same + 1);
        actions.push_back({"HELP: " + word, "СПРАВКА: " + word, [button] { button->activate(); }, ""});
      }
      if (auto screen = MSFormScreen::create(en ? "PROFILE SETTINGS" : "НАСТРОЙКИ ПРОФИЛЯ", keep, std::move(fields),
                                             std::move(actions), nullptr, std::move(options)))
        screen->show();
    });
    return true;
  }
};
