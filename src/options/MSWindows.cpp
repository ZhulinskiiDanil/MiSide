#include "MSWindows.hpp"

#include "MSAlerts.hpp"
#include "MSOptionsData.hpp"
#include "MSOptionsScreen.hpp"
#include "MSOptionsText.hpp"

#include "../Theme.hpp"

#include <Geode/modify/GJDropDownLayer.hpp>

#include <deque>
#include <unordered_set>

using namespace geode::prelude;

namespace
{
  // How often a screen looks at its window for changes
  constexpr float WATCH_TIME = 0.25f;
  // A toggle's text is the label right of it, about level with it
  constexpr float LABEL_RISE = 14;
  // The windows that stay delegates of GD's managers (an account being saved, a request being sent) are kept after
  // their screen goes, the last few of them
  constexpr size_t KEPT = 12;

  std::deque<Ref<CCNode>>& kept()
  {
    static std::deque<Ref<CCNode>> s_kept;
    return s_kept;
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
    uintptr_t offset = 0;
    char const* en = nullptr;
    char const* ru = nullptr;
    char const* infoEn = "";
    char const* infoRu = "";
    bool skip = false;
    std::function<void()> action;
  };

  // GD's windows: GJDropDownLayer::exitLayer, FLAlertLayer-style closes (UploadPopup::onClose, as GD's popups share it)
  constexpr uintptr_t DROP_DOWN_EXIT = 0x257320;
  constexpr uintptr_t POPUP_CLOSE = 0x84670;

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

  void update(float dt) override
  {
    MSOptionsScreen::update(dt);
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
    if (!node || !node->isVisible())
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
      if (ms::options::runs(item, m_known[i].offset))
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
      if (!entry && (ms::options::runs(item, DROP_DOWN_EXIT) || ms::options::runs(item, POPUP_CLOSE)))
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
      if (row.text.empty())
        row.text = en ? "BUTTON" : "КНОПКА";
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

  void click(Ref<CCMenuItem> item, std::function<void()> const& action)
  {
    if (action)
      return action();
    this->press(m_window, [item] { item->activate(); });
  }

  std::string m_title;
  Ref<CCNode> m_window;
  std::vector<Known> m_known;
  Group m_group;
  std::string m_shown;
  float m_watch = 0;
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
    if (!links || !ms::options::restyling())
      return GJDropDownLayer::showLayer(instant);
    Ref<GJDropDownLayer> self = this;
    ms::options::openLinks(links);
    this->removeFromParentAndCleanup(true);
  }
};

namespace ms::options
{
  void openLinks(URLViewLayer* links)
  {
    if (auto screen = MSLinksScreen::create(links))
      screen->show();
  }

  void openSongInfo(int songID)
  {
    auto window = SongInfoLayer::create(songID);
    std::vector<Known> known = {
      {0x494440, nullptr, nullptr, "Downloads the song to play it in the editor's levels.",
       "Скачивает песню, чтобы использовать её в уровнях редактора."},
      {0x4942b0, "MENU MUSIC", "МУЗЫКА В МЕНЮ", "Plays this song in GD's menu.", "Эта песня играет в меню GD."},
      {0x494380, "PRACTICE MUSIC", "МУЗЫКА В ПРАКТИКЕ", "Plays this song in practice mode.",
       "Эта песня играет в режиме практики."},
      {0x494500, "BPM", "BPM", "GD's beat guide for this song.", "Метки ритма GD для этой песни."},
      {0x494470, nullptr, nullptr, "The artist's page.", "Страница автора."},
      {0x4944a0, nullptr, nullptr, "The artist's YouTube.", "YouTube автора."},
      {0x4944d0, nullptr, nullptr, "The artist's page.", "Страница автора."},
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
          {0xcaec0, "PLAY / STOP", "ИГРАТЬ / СТОП", "Listens to the song.", "Послушать песню."},
          {0xc9c10, "INFO", "О ПЕСНЕ", "The song's page.", "Страница песни."},
          {0xcae60, "USE", "ВЫБРАТЬ", "", ""},
          {0xcabc0, "DOWNLOAD", "СКАЧАТЬ", "", ""},
          {0xcaaa0, "CANCEL DOWNLOAD", "ОТМЕНИТЬ ЗАГРУЗКУ", "", ""},
          {0xcab30, "REFRESH INFO", "ОБНОВИТЬ ДАННЫЕ", "", ""},
          {0xca220, "MORE", "ЕЩЁ", "", ""},
          {0xc9f00, "DELETE", "УДАЛИТЬ", "Deletes the song from this device.", "Удаляет песню с устройства."},
          {0xb48e0, "DELETE", "УДАЛИТЬ", "Deletes the song from this device.", "Удаляет песню с устройства."},
        };
        if (auto screen = MSWindowScreen::create(ms::options::upper(title), keepCell, std::move(known), nullptr, false))
          screen->show();
      };
      return true;
    };
    std::vector<Known> known = {
      {0x2b7a10, "PREVIOUS PAGE", "ПРЕДЫДУЩАЯ СТРАНИЦА", "", ""},
      {0x2b7a00, "NEXT PAGE", "СЛЕДУЮЩАЯ СТРАНИЦА", "", ""},
      {0x2b7a20, "DELETE ALL", "УДАЛИТЬ ВСЕ", "Deletes every downloaded custom song.", "Удаляет все скачанные песни."},
    };
    if (auto screen = MSWindowScreen::create(en ? "SAVED SONGS" : "СОХРАНЁННЫЕ ПЕСНИ", GJSongBrowser::create(),
                                             std::move(known), [] { dropDown(GJSongBrowser::create()); }, true, song))
      screen->show();
  }

  void openSupport()
  {
    std::vector<Known> known = {
      {0x4ca0f0, "LINKS", "ССЫЛКИ", "GD's FAQ, the editor guide and the rating system.",
       "Вопросы и ответы GD, руководство по редактору, система оценок."},
      {0x4c9ff0, "CONTACT", "СВЯЗАТЬСЯ", "How to write to RobTop Games' support.", "Как написать в поддержку RobTop Games."},
      {0x29e5e0, "PRIVACY POLICY", "КОНФИДЕНЦИАЛЬНОСТЬ", "RobTop Games' privacy policy.",
       "Политика конфиденциальности RobTop Games."},
      {0x29e600, "TERMS OF USE", "УСЛОВИЯ ИСПОЛЬЗОВАНИЯ", "RobTop Games' terms of use.",
       "Условия использования RobTop Games."},
      {0x4c9380, "SOUND EFFECTS", "ЗВУКОВЫЕ ЭФФЕКТЫ", "Who made GD's sound effects.", "Кто сделал звуковые эффекты GD."},
      {0x4c9d60, "LOW DETAIL MODE", "НИЗКАЯ ДЕТАЛИЗАЦИЯ ВЕЗДЕ",
       "Turns off a lot of visual effects for speed.", "Отключает многие визуальные эффекты ради скорости."},
      {0x4c9880, "REQUEST ACCESS", "ЗАПРОСИТЬ ДОСТУП", "Asks for moderator access (for the ones GD invited).",
       "Запрос прав модератора (для тех, кого GD пригласила)."},
      {0x4c9970, "GET REWARD", "ПОЛУЧИТЬ НАГРАДУ", "A reward GD has for you.", "Награда, которую GD для вас приготовила."},
      {0x335680, "ROBTOP GAMES", "ROBTOP GAMES", "RobTop Games' site.", "Сайт RobTop Games."},
      {0x4c9860, "COCOS2D-X", "COCOS2D-X", "The engine GD is made with.", "Движок, на котором сделана GD."},
    };
    auto en = MSScreen::english();
    if (auto screen = MSWindowScreen::create(en ? "SUPPORT" : "ПОДДЕРЖКА", SupportLayer::create(), std::move(known),
                                             [] { dropDown(SupportLayer::create()); }, true))
      screen->show();
  }

  void openAccountHelp()
  {
    std::vector<Known> known = {
      {0x7fa20, "REFRESH LOGIN", "ОБНОВИТЬ ВХОД", "Signs in again with your account.", "Заново входит в ваш аккаунт."},
      {0x7fa60, "ACCOUNT MANAGEMENT", "УПРАВЛЕНИЕ АККАУНТОМ", "GD's account page in the browser.",
       "Страница аккаунта GD в браузере."},
      {0x7fb70, "UNLINK ACCOUNT", "ОТВЯЗАТЬ АККАУНТ", "Unlinks this game from your account.",
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
      {0x7de60, "LOG IN", "ВОЙТИ", "Signs in with an account (GD's form).", "Вход в аккаунт (форма GD)."},
      {0x7dea0, "REGISTER", "ЗАРЕГИСТРИРОВАТЬСЯ", "Makes an account (GD's form).", "Создание аккаунта (форма GD)."},
      {0x7e020, "SAVE", "СОХРАНИТЬ", "Saves your data on the cloud.", "Сохраняет ваши данные в облаке."},
      {0x7e3b0, "LOAD", "ЗАГРУЗИТЬ", "Loads your data from the cloud, merged with what's here.",
       "Загружает данные из облака и объединяет их с текущими."},
      {0x7e630, "HELP", "ПОМОЩЬ", "GD's account help page.", "Страница помощи по аккаунту GD."},
      // GD's More hides the window first, then drops its own down: here, the mod's screen of it
      {0x7e750, "MORE", "ЕЩЁ", "Refresh the login, unlink, manage the account.",
       "Обновить вход, отвязать, управление аккаунтом.", false, [] { openAccountHelp(); }},
    };
    auto en = MSScreen::english();
    if (auto screen = MSWindowScreen::create(en ? "ACCOUNT" : "АККАУНТ", AccountLayer::create(), std::move(known),
                                             [] { dropDown(AccountLayer::create()); }, true))
      screen->show();
  }
} // namespace ms::options
