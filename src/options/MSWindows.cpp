#include "MSWindows.hpp"

#include "MSAlerts.hpp"
#include "MSOptionsData.hpp"
#include "MSOptionsScreen.hpp"
#include "MSOptionsText.hpp"

#include "../Theme.hpp"

#include <Geode/modify/GJDropDownLayer.hpp>

#include <cstring>
#include <deque>
#include <unordered_set>

using namespace geode::prelude;

namespace
{
  // How often a screen looks at its window for changes
  constexpr float WATCH_TIME = 0.25f;
  // A toggle's text is the label right of it, about level with it; a button with none takes the nearest this close
  constexpr float LABEL_RISE = 14;
  constexpr float NEAR_LABEL = 40;
  // The windows that stay delegates of GD's managers (an account being saved, a request being sent) are kept after
  // their screen goes, the last few of them
  constexpr size_t KEPT = 12;

  // Never destroyed: at the game's exit, releasing GD's windows from the mod's own unloading reaches other mods'
  // hooks on their destructors after those mods are gone (a crash on quitting)
  std::deque<Ref<CCNode>>& kept()
  {
    static auto s_kept = new std::deque<Ref<CCNode>>();
    return *s_kept;
  }

  void keep(CCNode* window)
  {
    auto& list = kept();
    list.emplace_back(window);
    while (list.size() > KEPT)
      list.pop_front();
  }

  CCPoint middle(CCNode* node) { return node->convertToWorldSpace(node->getContentSize() / 2); }

  std::string pick(char const* en, char const* ru, bool english)
  {
    auto text = english ? en : ru;
    return text ? text : "";
  }

  // A known button of a GD window: the function it runs, the mod's text for it (none: GD's own), what it does, and
  // whether it's left out (closing the window: BACK does that) or does something else here
  struct Known
  {
    // GD's function the button runs, where it is on this platform
    uintptr_t address = 0;
    char const* en = nullptr;
    char const* ru = nullptr;
    char const* infoEn = "";
    char const* infoRu = "";
    bool skip = false;
    std::function<void()> action;
  };

  // GD's own way out of a window: GJDropDownLayer::exitLayer (and its back arrow's), or an FLAlertLayer-style close
  // (UploadPopup::onClose, as GD's popups share it)
  bool closes(CCMenuItem* item)
  {
    if (ms::options::runs(item, ms::options::gd(&UploadPopup::onClose)))
      return true;
#ifdef GEODE_IS_WINDOWS
    // exitLayer is virtual: GD's buttons call it straight or through a thunk of MSVC's in GeometryDash.exe 2.2081
    auto base = geode::base::get();
    return ms::options::runs(item, base + 0x257320) || ms::options::runs(item, base + 0x257638);
#else
    // A virtual function's member pointer is its vtable slot: the same in GD's code as here
    return item && item->m_pfnSelector == menu_selector(GJDropDownLayer::exitLayer);
#endif
  }

  // The text on a button: its ButtonSprite's, else the first label in it
  std::string buttonText(CCMenuItem* item)
  {
    if (auto sprite = typeinfo_cast<CCMenuItemSprite*>(item))
    {
      if (auto button = typeinfo_cast<ButtonSprite*>(sprite->getNormalImage()); button && button->m_label)
        return ms::options::plain(button->m_label->getString());
    }
    return ms::options::textOf(item);
  }
} // namespace

namespace ms::options
{
  void dropDown(GJDropDownLayer* layer)
  {
    auto scene = CCDirector::get()->getRunningScene();
    if (!layer || !scene)
      return;
    scene->addChild(layer, std::max(scene->getHighestChildZ(), 105));
    layer->showLayer(false);
  }
} // namespace ms::options

// ! --- A GD window as a screen --- !

// GD's window made for this screen and never shown: its texts (labels, text areas) as paragraphs top to bottom, its
// buttons and toggles as rows (the known ones first, in their order, with the mod's texts; the others by where they
// are, with GD's). A row presses GD's own button. Whenever the window changes (a button shows or goes, a text
// changes, something loads), the screen is made again from it
class MSWindowScreen : public MSOptionsScreen
{
public:
  // A part of the window shown as one row that opens a screen of its own (a list's cell): its text, and what opens
  using Group = std::function<bool(CCNode* node, std::string& text, std::function<void()>& open)>;

  static MSWindowScreen* create(std::string const& title, CCNode* window, std::vector<Known> known,
                                std::function<void()> vanilla, bool keepWindow, Group group = nullptr)
  {
    auto screen = new MSWindowScreen();
    screen->m_group = std::move(group);
    if (screen->setup(title, window, std::move(known), std::move(vanilla), keepWindow))
    {
      screen->autorelease();
      return screen;
    }
    delete screen;
    return nullptr;
  }

  // A window shown under the screen: closing the screen closes it, it closing closes the screen
  void follow(std::function<void()> closeWindow)
  {
    m_follow = true;
    m_closeWindow = std::move(closeWindow);
  }

  void update(float dt) override
  {
    MSOptionsScreen::update(dt);
    if (m_follow && !this->isClosing() && !m_window->getParent())
    {
      m_closeWindow = nullptr;
      return this->close();
    }
    m_watch += dt;
    if (m_watch < WATCH_TIME || this->isClosing())
      return;
    m_watch = 0;
    if (this->describe() != m_shown)
    {
      this->resetOptions();
      this->build();
    }
  }

protected:
  struct Part
  {
    CCNode* node = nullptr;
    std::string text;
    std::function<void()> open;
  };

  struct Scan
  {
    std::vector<CCMenuItem*> items;
    std::vector<CCNode*> texts;
    std::vector<Part> parts;
    bool loading = false;
  };

  bool setup(std::string const& title, CCNode* window, std::vector<Known> known, std::function<void()> vanilla,
             bool keepWindow)
  {
    if (!window || !this->initOptions(title))
      return false;
    m_title = title;
    m_window = this->drive(window);
    if (keepWindow)
      keep(window);
    m_known = std::move(known);
    if (vanilla)
      this->addBottomButton(english() ? "GD WINDOW" : "ОКНО GD", Theme::get().screenButtonWidth, std::move(vanilla));
    this->build();
    return true;
  }

  void scan(CCNode* node, Scan& out)
  {
    // The window itself may be hidden (GD's windows that drop down are, until shown): only what's in it counts
    if (!node || (node != m_window && !node->isVisible()))
      return;
    if (m_group)
    {
      Part part;
      if (m_group(node, part.text, part.open))
      {
        part.node = node;
        out.parts.push_back(std::move(part));
        return;
      }
    }
    if (auto item = typeinfo_cast<CCMenuItem*>(node))
    {
      out.items.push_back(item);
      return;
    }
    if (typeinfo_cast<LoadingCircle*>(node))
    {
      out.loading = true;
      return;
    }
    // A text field's own labels (its text, its placeholder) aren't the window's texts
    if (typeinfo_cast<CCTextInputNode*>(node))
      return;
    if (typeinfo_cast<TextArea*>(node) || typeinfo_cast<CCLabelBMFont*>(node))
    {
      out.texts.push_back(node);
      return;
    }
    for (auto child : CCArrayExt<CCNode*>(node->getChildren()))
      this->scan(child, out);
  }

  // What the window shows now, to see when it changes
  std::string describe()
  {
    Scan found;
    this->scan(m_window, found);
    std::string out = found.loading ? "L" : "";
    for (auto item : found.items)
      out += fmt::format("|{}{}{}", static_cast<void*>(item), item->isEnabled() ? 1 : 0, buttonText(item));
    for (auto text : found.texts)
      out += "|" + ms::options::textOf(text);
    for (auto const& part : found.parts)
      out += fmt::format("|{}{}", static_cast<void*>(part.node), part.text);
    return out;
  }

  Known const* known(CCMenuItem* item, int& order)
  {
    for (size_t i = 0; i < m_known.size(); i++)
    {
      if (ms::options::runs(item, m_known[i].address))
      {
        order = static_cast<int>(i);
        return &m_known[i];
      }
    }
    order = -1;
    return nullptr;
  }

  void build()
  {
    auto en = english();
    Scan found;
    this->scan(m_window, found);
    m_shown = this->describe();

    struct Row
    {
      Ref<CCMenuItem> item;
      std::string text;
      std::string info;
      int order = 0;
      CCPoint at;
      std::function<void()> action;
    };
    std::vector<Row> rows;
    std::unordered_set<CCNode*> used;
    for (auto item : found.items)
    {
      if (!item->isEnabled())
        continue;
      int order = -1;
      auto entry = this->known(item, order);
      if (entry && entry->skip)
        continue;
      if (!entry && closes(item))
        continue;
      // Another mod's button in a window that isn't shown: its code expects the window on screen (one crashed the
      // game so, in the self-test). GD WINDOW has it
      if (!entry && !m_follow && !ms::options::runsGD(item))
        continue;
      Row row;
      row.item = item;
      row.at = middle(item);
      row.order = order >= 0 ? order : 1000;
      if (entry)
      {
        row.text = pick(entry->en, entry->ru, en);
        row.info = pick(entry->infoEn, entry->infoRu, en);
        row.action = entry->action;
      }
      if (row.text.empty())
        row.text = buttonText(item);
      // A toggle's text: the label right of it
      if (typeinfo_cast<CCMenuItemToggler*>(item) || row.text.empty())
      {
        CCNode* best = nullptr;
        auto bestDistance = 1e9f;
        for (auto text : found.texts)
        {
          if (used.contains(text) || !typeinfo_cast<CCLabelBMFont*>(text))
            continue;
          auto at = middle(text);
          auto left = text->convertToWorldSpace(CCPointZero).x;
          if (left < row.at.x - 4 || std::abs(at.y - row.at.y) > LABEL_RISE)
            continue;
          if (left - row.at.x < bestDistance)
          {
            bestDistance = left - row.at.x;
            best = text;
          }
        }
        if (best)
        {
          used.insert(best);
          if (!entry || !(en ? entry->en : entry->ru))
            row.text = ms::options::textOf(best);
        }
      }
      // Still none: the nearest label any way round, close by
      if (row.text.empty())
      {
        CCNode* best = nullptr;
        auto bestDistance = NEAR_LABEL;
        for (auto text : found.texts)
        {
          if (used.contains(text) || !typeinfo_cast<CCLabelBMFont*>(text))
            continue;
          auto distance = middle(text).getDistance(row.at);
          if (distance < bestDistance)
          {
            bestDistance = distance;
            best = text;
          }
        }
        if (best)
        {
          used.insert(best);
          row.text = ms::options::textOf(best);
        }
      }
      if (row.text.empty())
      {
        row.text = ms::options::runsGD(item) ? (en ? "BUTTON" : "КНОПКА") : (en ? "MOD'S BUTTON" : "КНОПКА МОДА");
      }
      if (ms::options::selfTesting() && !entry)
      {
        uintptr_t address = 0;
        std::memcpy(&address, &item->m_pfnSelector, sizeof(address));
        auto base = geode::base::get();
        auto at = row.at;
        log::info("SELFTEST row \"{}\": {} {} at {:.0f},{:.0f}", row.text, typeid(*item).name(),
                  ms::options::runsGD(item) ? fmt::format("GD+{:#x}", address - base) : "not GD's",
                  at.x, at.y);
      }
      rows.push_back(std::move(row));
    }
    std::stable_sort(rows.begin(), rows.end(), [](Row const& a, Row const& b) {
      if (a.order != b.order)
        return a.order < b.order;
      if (std::abs(a.at.y - b.at.y) > 2)
        return a.at.y > b.at.y;
      return a.at.x < b.at.x;
    });

    // The texts top to bottom, but the window's own title (the screen has it)
    std::vector<CCNode*> texts;
    for (auto text : found.texts)
    {
      if (!used.contains(text))
        texts.push_back(text);
    }
    std::stable_sort(texts.begin(), texts.end(), [](CCNode* a, CCNode* b) {
      auto ya = a->convertToWorldSpace({0, a->getContentSize().height}).y;
      auto yb = b->convertToWorldSpace({0, b->getContentSize().height}).y;
      return ya > yb;
    });
    for (auto text : texts)
    {
      auto string = ms::options::textOf(text);
      if (string.empty() || ms::options::upper(string) == ms::options::upper(m_title))
        continue;
      this->addText(string);
    }
    if (found.loading)
      this->addText(en ? "Wait..." : "Подождите...");

    // The parts first, top to bottom
    std::stable_sort(found.parts.begin(), found.parts.end(), [](Part const& a, Part const& b) {
      return middle(a.node).y > middle(b.node).y;
    });
    for (auto const& part : found.parts)
      m_column->addRow(ms::options::upper(part.text), part.open);

    for (auto const& row : rows)
    {
      auto action = row.action;
      auto item = row.item;
      if (auto toggler = typeinfo_cast<CCMenuItemToggler*>(item.data()))
      {
        MSPauseColumn::Choice choice;
        choice.value = [toggler = Ref(toggler), en] {
          return std::string(toggler->isToggled() ? (en ? "ON" : "ВКЛ") : (en ? "OFF" : "ВЫКЛ"));
        };
        choice.change = [this, item, action](int) { this->click(item, action); };
        m_column->addChoice(ms::options::upper(row.text), std::move(choice));
      }
      else
        m_column->addRow(ms::options::upper(row.text), [this, item, action] { this->click(item, action); });
      m_column->setInfo(row.info);
    }
    this->finishOptions();
  }

  void closed() override
  {
    if (m_closeWindow && m_window->getParent())
      m_closeWindow();
  }

  void click(Ref<CCMenuItem> item, std::function<void()> const& action)
  {
    if (ms::options::selfTesting())
      log::info("SELFTEST click {} enabled={} children before={}", typeid(*item.data()).name(), item->isEnabled(),
                m_window->getChildrenCount());
    if (action)
      return action();
    this->press(m_window, [item] { item->activate(); });
    if (ms::options::selfTesting())
    {
      std::string found;
      if (auto scene = CCDirector::get()->getRunningScene())
      {
        for (auto child : CCArrayExt<CCNode*>(scene->getChildren()))
          found += std::string(typeid(*child).name()) + " ";
      }
      log::info("SELFTEST after click: window children {}, scene: {}", m_window->getChildrenCount(), found);
    }
  }

  std::string m_title;
  Ref<CCNode> m_window;
  std::vector<Known> m_known;
  Group m_group;
  bool m_follow = false;
  std::function<void()> m_closeWindow;
  std::string m_shown;
  float m_watch = 0;
};

// ! --- A reward --- !

// GD's reward chest (the support's reward): GD's chest and its animation stay in sight, the mod's column over it opens
// and closes it as GD's buttons do
class MSRewardOverlay : public MSOptionsScreen
{
public:
  static MSRewardOverlay* create(RewardUnlockLayer* reward)
  {
    auto screen = new MSRewardOverlay();
    if (screen->setup(reward))
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
    if (!m_reward->getParent())
      return this->close();
    if (this->state() != m_state)
    {
      this->resetOptions();
      this->build();
    }
  }

protected:
  static bool usable(CCMenuItem* item) { return item && item->isVisible() && item->isEnabled(); }

  int state() { return (usable(m_reward->m_rewardBtn) ? 1 : 0) | (usable(m_reward->m_closeBtn) ? 2 : 0); }

  bool setup(RewardUnlockLayer* reward)
  {
    if (!reward || !this->initOptions(english() ? "REWARD" : "НАГРАДА", true))
      return false;
    m_reward = reward;
    this->build();
    return true;
  }

  void build()
  {
    auto en = english();
    m_state = this->state();
    if (m_state & 1)
    {
      Ref<CCMenuItem> button = m_reward->m_rewardBtn;
      m_column->addRow(en ? "OPEN" : "ОТКРЫТЬ", [button] { button->activate(); });
    }
    if (m_state & 2)
    {
      Ref<CCMenuItem> button = m_reward->m_closeBtn;
      m_column->addRow(en ? "COLLECT" : "ЗАБРАТЬ", [button] { button->activate(); });
    }
    if (!m_state)
      this->addText(en ? "Wait..." : "Подождите...");
    this->finishOptions();
  }

  // BACK: GD's close, when GD has it
  void closed() override
  {
    if (m_reward->getParent() && usable(m_reward->m_closeBtn))
      m_reward->m_closeBtn->activate();
  }

  Ref<RewardUnlockLayer> m_reward;
  int m_state = -1;
};

// ! --- The soundtracks --- !

// GD's official songs (as its soundtracks window lists them): each opens its window
class MSSongsScreen : public MSOptionsScreen
{
public:
  static MSSongsScreen* create()
  {
    auto screen = new MSSongsScreen();
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
    if (!this->initOptions(en ? "SOUNDTRACKS" : "САУНДТРЕКИ"))
      return false;
    this->addBottomButton(en ? "GD WINDOW" : "ОКНО GD", Theme::get().screenButtonWidth,
                          [] { ms::options::dropDown(SongsLayer::create()); });
    // SongsLayer::customSetup's list: the songs 0 to 21, then the practice song (-1)
    std::vector<int> songs;
    for (int id = 0; id <= 21; id++)
      songs.push_back(id);
    songs.push_back(-1);
    for (auto id : songs)
    {
      std::string title = LevelTools::getAudioTitle(id);
      std::string artist = LevelTools::nameForArtist(LevelTools::artistForAudio(id));
      m_column->addRow(ms::options::upper(title), [id] { ms::options::openSongInfo(id); });
      m_column->setInfo((en ? "By " : "Автор: ") + artist);
    }
    this->finishOptions();
    return true;
  }
};

// ! --- How to play --- !

// GD's tutorial: its pictures, page by page, with its words (in Russian, the mod's)
class MSTutorialScreen : public MSOptionsScreen
{
public:
  static MSTutorialScreen* create()
  {
    auto screen = new MSTutorialScreen();
    if (screen->setup())
    {
      screen->autorelease();
      return screen;
    }
    delete screen;
    return nullptr;
  }

  ~MSTutorialScreen() override { this->unload(); }

protected:
  // TutorialLayer::loadPage's pages
  static constexpr int PAGES = 5;
  // The picture's room over the words
  static constexpr float PICTURE_HEIGHT = 120;

  bool setup()
  {
    auto en = english();
    if (!this->initOptions(en ? "HOW TO PLAY" : "КАК ИГРАТЬ"))
      return false;
    this->addBottomButton(en ? "GD WINDOW" : "ОКНО GD", Theme::get().screenButtonWidth, [] {
      if (auto layer = TutorialLayer::create())
        layer->show();
    });
    this->showPage(1);
    return true;
  }

  static char const* words(int page, bool en)
  {
    switch (page)
    {
    case 1:
      return en ? "Tap the screen to jump. Hold down to keep jumping."
                : "Нажмите на экран, чтобы прыгнуть. Удерживайте, чтобы прыгать дальше.";
    case 2:
      return en ? "Hold to fly up. Release to fly down." : "Удерживайте, чтобы лететь вверх. Отпустите — вниз.";
    case 3:
      return en ? "You can enter practice mode from the pause menu. Practice mode lets you place checkpoints."
                : "Режим практики включается в меню паузы. В нём можно ставить контрольные точки.";
    case 4:
      return en ? "You can place checkpoints manually, or use the auto-checkpoint feature. Tap the delete button to "
                  "remove your last checkpoint."
                : "Контрольные точки можно ставить вручную или включить автоточки. Кнопка удаления убирает последнюю "
                  "точку.";
    default:
      return en ? "Jump Orbs activate when you are on top of them. Tap while touching an orb to interact with it and "
                  "use its effect."
                : "Сферы прыжка срабатывают, когда вы на них. Нажмите, касаясь сферы, чтобы использовать её эффект.";
    }
  }

  std::string file(int page) const { return fmt::format("tutorial_{:02}.png", page); }

  // GD lets each page's picture go when the next one comes
  void unload()
  {
    if (m_page > 0)
      CCTextureCache::sharedTextureCache()->removeTextureForKey(this->file(m_page).c_str());
  }

  void showPage(int page)
  {
    auto en = english();
    auto const& theme = Theme::get();
    this->unload();
    if (m_picture)
    {
      m_picture->removeFromParent();
      m_picture = nullptr;
    }
    this->resetOptions();
    m_page = page;
    if (auto picture = CCSprite::create(this->file(page).c_str()))
    {
      auto size = picture->getContentSize();
      auto scale = std::min(theme.optionsRowWidth / std::max(size.width, 1.f), PICTURE_HEIGHT / std::max(size.height, 1.f));
      picture->setScale(scale);
      picture->setAnchorPoint({0, 1});
      picture->setPosition({theme.screenLeft, m_top + fromTop()});
      m_mainLayer->addChild(picture, 10);
      m_picture = picture;
      m_top -= size.height * scale + 4;
    }
    this->addText(fmt::format("{} / {}", page, PAGES));
    this->addText(words(page, en));
    if (page < PAGES)
      m_column->addRow(en ? "NEXT" : "ДАЛЕЕ", [this] { this->showPage(m_page + 1); });
    else
      m_column->addRow(en ? "DONE" : "ГОТОВО", [this] { this->close(); });
    if (page > 1)
      m_column->addRow(en ? "PREVIOUS" : "НАЗАД К ПРЕДЫДУЩЕЙ", [this] { this->showPage(m_page - 1); });
    this->finishOptions();
  }

  // Left and Right turn the pages too
  bool onKey(enumKeyCodes key) override
  {
    if (key == KEY_Right || key == CONTROLLER_Right)
    {
      if (m_page < PAGES)
        this->showPage(m_page + 1);
      return true;
    }
    if (key == KEY_Left || key == CONTROLLER_Left)
    {
      if (m_page > 1)
        this->showPage(m_page - 1);
      return true;
    }
    return MSOptionsScreen::onKey(key);
  }

  int m_page = 0;
  CCNode* m_picture = nullptr;
};

// ! --- Links --- !

// GD's list of links: each opens in the browser, as GD's does
class MSLinksScreen : public MSOptionsScreen
{
public:
  static MSLinksScreen* create(URLViewLayer* links)
  {
    auto screen = new MSLinksScreen();
    if (screen->setup(links))
    {
      screen->autorelease();
      return screen;
    }
    delete screen;
    return nullptr;
  }

protected:
  bool setup(URLViewLayer* links)
  {
    auto en = english();
    if (!links || !this->initOptions(en ? "LINKS" : "ССЫЛКИ"))
      return false;
    if (auto objects = links->m_urlObjects)
    {
      for (auto object : CCArrayExt<CCObject*>(objects))
      {
        auto url = typeinfo_cast<CCURLObject*>(object);
        if (!url)
          continue;
        std::string address = url->m_url;
        m_column->addRow(ms::options::upper(url->m_name), [address] {
          CCApplication::sharedApplication()->openURL(address.c_str());
        });
        m_column->setInfo(address);
      }
    }
    this->finishOptions();
    return true;
  }
};

// While the settings screens are open, GD's lists of links come up as the mod's screen
class $modify(MSLinksDropDown, GJDropDownLayer)
{
  void showLayer(bool instant)
  {
    auto links = typeinfo_cast<URLViewLayer*>(this);
    if (ms::options::selfTesting())
      log::info("SELFTEST showLayer {} links={} restyling={}", typeid(*this).name(), links != nullptr,
                ms::options::restyling());
    if (!links || !ms::options::restyling())
      return GJDropDownLayer::showLayer(instant);
    Ref<GJDropDownLayer> self = this;
    ms::options::openLinks(links);
    this->removeFromParentAndCleanup(true);
  }
};

namespace ms::options
{
  void rewardOverlay(RewardUnlockLayer* reward)
  {
    if (auto screen = MSRewardOverlay::create(reward))
      screen->show();
  }

  void mirrorPopup(FLAlertLayer* popup)
  {
    auto en = MSScreen::english();
    std::string title;
    std::vector<Known> known;
    if (typeinfo_cast<UploadActionPopup*>(popup))
    {
      title = en ? "REQUEST" : "ЗАПРОС";
      known = {{ms::options::gd(&UploadActionPopup::onClose), "CLOSE", "ЗАКРЫТЬ", "", ""}};
    }
    else if (typeinfo_cast<UISaveLoadLayer*>(popup))
      title = en ? "SAVE / LOAD UI" : "СОХРАНИТЬ / ЗАГРУЗИТЬ";
    else if (typeinfo_cast<UpdateAccountSettingsPopup*>(popup))
    {
      title = en ? "PROFILE SETTINGS" : "НАСТРОЙКИ ПРОФИЛЯ";
      known = {{ms::options::gd(&UpdateAccountSettingsPopup::onClose), "CLOSE", "ЗАКРЫТЬ", "", ""}};
    }
    else
      return;
    Ref<FLAlertLayer> keep = popup;
    if (auto screen = MSWindowScreen::create(title, popup, std::move(known), nullptr, false))
    {
      screen->follow([keep] { keep->keyBackClicked(); });
      screen->show();
    }
  }

  void openLinks(URLViewLayer* links)
  {
    auto screen = MSLinksScreen::create(links);
    if (ms::options::selfTesting())
      log::info("SELFTEST openLinks made={} objects={}", screen != nullptr,
                links && links->m_urlObjects ? links->m_urlObjects->count() : 0u);
    if (screen)
      screen->show();
  }

  void openSongInfo(int songID)
  {
    auto window = SongInfoLayer::create(songID);
    std::vector<Known> known = {
      {ms::options::gd(&SongInfoLayer::onDownload), "DOWNLOAD SOUNDTRACK", "СКАЧАТЬ САУНДТРЕК", "Downloads the song to play it in the editor's levels.",
       "Скачивает песню, чтобы использовать её в уровнях редактора."},
      {ms::options::gd(&SongInfoLayer::onMenuMusic), "MENU MUSIC", "МУЗЫКА В МЕНЮ", "Plays this song in GD's menu.", "Эта песня играет в меню GD."},
      {ms::options::gd(&SongInfoLayer::onPracticeMusic), "PRACTICE MUSIC", "МУЗЫКА В ПРАКТИКЕ", "Plays this song in practice mode.",
       "Эта песня играет в режиме практики."},
      {ms::options::gd(&SongInfoLayer::onBPM), "BPM", "BPM", "GD's beat guide for this song.", "Метки ритма GD для этой песни."},
      {ms::options::gd(&SongInfoLayer::onNG), nullptr, nullptr, "The artist's page.", "Страница автора."},
      {ms::options::gd(&SongInfoLayer::onYT), nullptr, nullptr, "The artist's YouTube.", "YouTube автора."},
      {ms::options::gd(&SongInfoLayer::onFB), nullptr, nullptr, "The artist's page.", "Страница автора."},
    };
    std::string title = LevelTools::getAudioTitle(songID);
    if (auto screen = MSWindowScreen::create(ms::options::upper(title), window, std::move(known),
                                             [songID] {
                                               if (auto layer = SongInfoLayer::create(songID))
                                                 layer->show();
                                             },
                                             false))
      screen->show();
  }

  void openSoundtracks()
  {
    if (auto screen = MSSongsScreen::create())
      screen->show();
  }

  void openTutorial()
  {
    if (auto screen = MSTutorialScreen::create())
      screen->show();
  }

  void openSavedSongs()
  {
    auto en = MSScreen::english();
    // A song's own buttons (CustomSongWidget's, the cell's delete)
    auto song = [](CCNode* node, std::string& text, std::function<void()>& open) {
      auto cell = typeinfo_cast<CustomSongCell*>(node);
      if (!cell)
        return false;
      auto info = cell->m_songInfoObject;
      text = info ? std::string(info->m_songName) : ms::options::textOf(cell);
      Ref<CustomSongCell> keepCell = cell;
      open = [keepCell, title = text] {
        std::vector<Known> known = {
          {ms::options::gd(&CustomSongWidget::onPlayback), "PLAY / STOP", "ИГРАТЬ / СТОП", "Listens to the song.", "Послушать песню."},
          {ms::options::gd(&CustomSongWidget::onInfo), "INFO", "О ПЕСНЕ", "The song's page.", "Страница песни."},
          {ms::options::gd(&CustomSongWidget::onSelect), "USE", "ВЫБРАТЬ", "", ""},
          {ms::options::gd(&CustomSongWidget::onDownload), "DOWNLOAD", "СКАЧАТЬ", "", ""},
          {ms::options::gd(&CustomSongWidget::onCancelDownload), "CANCEL DOWNLOAD", "ОТМЕНИТЬ ЗАГРУЗКУ", "", ""},
          {ms::options::gd(&CustomSongWidget::onGetSongInfo), "REFRESH INFO", "ОБНОВИТЬ ДАННЫЕ", "", ""},
          {ms::options::gd(&CustomSongWidget::onMore), "MORE", "ЕЩЁ", "", ""},
          {ms::options::gd(&CustomSongWidget::onDelete), "DELETE", "УДАЛИТЬ", "Deletes the song from this device.", "Удаляет песню с устройства."},
          {ms::options::gd(&CustomSongCell::onDelete), "DELETE", "УДАЛИТЬ", "Deletes the song from this device.", "Удаляет песню с устройства."},
        };
        if (auto screen = MSWindowScreen::create(ms::options::upper(title), keepCell, std::move(known), nullptr, false))
          screen->show();
      };
      return true;
    };
    std::vector<Known> known = {
      {ms::options::gd(&GJSongBrowser::onPrevPage), "PREVIOUS PAGE", "ПРЕДЫДУЩАЯ СТРАНИЦА", "", ""},
      {ms::options::gd(&GJSongBrowser::onNextPage), "NEXT PAGE", "СЛЕДУЮЩАЯ СТРАНИЦА", "", ""},
      {ms::options::gd(&GJSongBrowser::onDeleteAll), "DELETE ALL", "УДАЛИТЬ ВСЕ", "Deletes every downloaded custom song.", "Удаляет все скачанные песни."},
    };
    if (auto screen = MSWindowScreen::create(en ? "SAVED SONGS" : "СОХРАНЁННЫЕ ПЕСНИ", GJSongBrowser::create(),
                                             std::move(known), [] { dropDown(GJSongBrowser::create()); }, true, song))
      screen->show();
  }

  void openSupport()
  {
    std::vector<Known> known = {
      {ms::options::gd(&SupportLayer::onLinks), "LINKS", "ССЫЛКИ", "GD's FAQ, the editor guide and the rating system.",
       "Вопросы и ответы GD, руководство по редактору, система оценок."},
      {ms::options::gd(&SupportLayer::onEmail), "CONTACT", "СВЯЗАТЬСЯ", "How to write to RobTop Games' support.", "Как написать в поддержку RobTop Games."},
      {ms::options::gd(&TOSPopup::onPrivacy), "PRIVACY POLICY", "КОНФИДЕНЦИАЛЬНОСТЬ", "RobTop Games' privacy policy.",
       "Политика конфиденциальности RobTop Games."},
      {ms::options::gd(&TOSPopup::onTOS), "TERMS OF USE", "УСЛОВИЯ ИСПОЛЬЗОВАНИЯ", "RobTop Games' terms of use.",
       "Условия использования RobTop Games."},
      {ms::options::gd(&SupportLayer::onSFX), "SOUND EFFECTS", "ЗВУКОВЫЕ ЭФФЕКТЫ", "Who made GD's sound effects.", "Кто сделал звуковые эффекты GD."},
      {ms::options::gd(&SupportLayer::onLowDetail), "LOW DETAIL MODE", "НИЗКАЯ ДЕТАЛИЗАЦИЯ ВЕЗДЕ",
       "Turns off a lot of visual effects for speed.", "Отключает многие визуальные эффекты ради скорости."},
      {ms::options::gd(&SupportLayer::onRequestAccess), "REQUEST ACCESS", "ЗАПРОСИТЬ ДОСТУП", "Asks for moderator access (for the ones GD invited).",
       "Запрос прав модератора (для тех, кого GD пригласила)."},
      {ms::options::gd(&SupportLayer::onGetReward), "GET REWARD", "ПОЛУЧИТЬ НАГРАДУ", "A reward GD has for you.", "Награда, которую GD для вас приготовила."},
      {ms::options::gd(&SupportLayer::onRobTop), "ROBTOP GAMES", "ROBTOP GAMES", "RobTop Games' site.", "Сайт RobTop Games."},
      {ms::options::gd(&SupportLayer::onCocos2d), "COCOS2D-X", "COCOS2D-X", "The engine GD is made with.", "Движок, на котором сделана GD."},
    };
    auto en = MSScreen::english();
    if (auto screen = MSWindowScreen::create(en ? "SUPPORT" : "ПОДДЕРЖКА", SupportLayer::create(), std::move(known),
                                             [] { dropDown(SupportLayer::create()); }, true))
      screen->show();
  }

  void openAccountHelp()
  {
    std::vector<Known> known = {
      {ms::options::gd(&AccountHelpLayer::onReLogin), "REFRESH LOGIN", "ОБНОВИТЬ ВХОД", "Signs in again with your account.", "Заново входит в ваш аккаунт."},
      {ms::options::gd(&AccountHelpLayer::onAccountManagement), "ACCOUNT MANAGEMENT", "УПРАВЛЕНИЕ АККАУНТОМ", "GD's account page in the browser.",
       "Страница аккаунта GD в браузере."},
      {ms::options::gd(&AccountHelpLayer::onUnlink), "UNLINK ACCOUNT", "ОТВЯЗАТЬ АККАУНТ", "Unlinks this game from your account.",
       "Отвязывает игру от вашего аккаунта."},
    };
    auto en = MSScreen::english();
    if (auto screen = MSWindowScreen::create(en ? "ACCOUNT" : "АККАУНТ", AccountHelpLayer::create(), std::move(known),
                                             [] { dropDown(AccountHelpLayer::create()); }, true))
      screen->show();
  }

  void openAccount()
  {
    std::vector<Known> known = {
      {ms::options::gd(&AccountLayer::onLogin), "LOG IN", "ВОЙТИ", "Signs in with an account (GD's form).", "Вход в аккаунт (форма GD)."},
      {ms::options::gd(&AccountLayer::onRegister), "REGISTER", "ЗАРЕГИСТРИРОВАТЬСЯ", "Makes an account (GD's form).", "Создание аккаунта (форма GD)."},
      {ms::options::gd(&AccountLayer::onBackup), "SAVE", "СОХРАНИТЬ", "Saves your data on the cloud.", "Сохраняет ваши данные в облаке."},
      {ms::options::gd(&AccountLayer::onSync), "LOAD", "ЗАГРУЗИТЬ", "Loads your data from the cloud, merged with what's here.",
       "Загружает данные из облака и объединяет их с текущими."},
      {ms::options::gd(&AccountLayer::onHelp), "HELP", "ПОМОЩЬ", "GD's account help page.", "Страница помощи по аккаунту GD."},
      // GD's More hides the window first, then drops its own down: here, the mod's screen of it
      {ms::options::gd(&AccountLayer::onMore), "MORE", "ЕЩЁ", "Refresh the login, unlink, manage the account.",
       "Обновить вход, отвязать, управление аккаунтом.", false, [] { openAccountHelp(); }},
    };
    auto en = MSScreen::english();
    if (auto screen = MSWindowScreen::create(en ? "ACCOUNT" : "АККАУНТ", AccountLayer::create(), std::move(known),
                                             [] { dropDown(AccountLayer::create()); }, true))
      screen->show();
  }
} // namespace ms::options
