#include "Theme.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/LoadingLayer.hpp>

#include <Geode/fmod/fmod.hpp>
#include <Geode/utils/file.hpp>

#include <filesystem>
#include <optional>

using namespace geode::prelude;

namespace
{
  // GD's loading screen's own nodes by Geode's IDs (loader/src/ids/LoadingLayer.cpp): hidden, never removed
  constexpr char const* VANILLA[] = {"bg-texture",  "gd-logo",      "robtop-logo", "cocos2d-logo",
                                     "fmod-logo",   "loading-text", "text-area",   "progress-slider"};
  // The bar's fill catches up with the real progress this fast (its share a second)
  constexpr float CATCH_UP = 6;
  // The item font's capitals: 11.7 pt high at scale 1, their middle 1.88 pt over the label's
  constexpr float FONT_CAP = 11.7f;
  constexpr float FONT_RAISE = 1.88f;

  // The mod's resources aren't in cocos' search paths this early (Geode adds them later in the loading): a file by
  // its full path, under its plain name. GD's file utils add the texture quality's -hd / -uhd themselves (a name that
  // has it already would get it twice and not be found)
  std::string resource(std::string const& name)
  {
    std::error_code error;
    auto plain = Mod::get()->getResourcesDir() / name;
    return std::filesystem::exists(plain, error) ? plain.string() : std::string();
  }

  // The mod's font, if its atlas really loads: read the .fnt GD resolves, load the page it names. Else empty (GD's
  // font instead): a label made from a font whose atlas didn't load crashes cocos
  std::string fontFile()
  {
    static std::optional<std::string> s_font;
    if (s_font)
      return *s_font;
    s_font = std::string();
    auto fnt = resource("miside-font.fnt");
    if (fnt.empty())
      return *s_font;
    auto files = CCFileUtils::sharedFileUtils();
    std::string resolved = files->fullPathForFilename(fnt.c_str(), false);
    auto text = file::readString(std::filesystem::path(resolved));
    if (!text)
      return *s_font;
    auto at = text.unwrap().find("file=\"");
    if (at == std::string::npos)
      return *s_font;
    auto start = at + 6;
    auto end = text.unwrap().find('"', start);
    auto page = text.unwrap().substr(start, end - start);
    std::string atlas = files->fullPathFromRelativeFile(page.c_str(), resolved.c_str());
    if (CCTextureCache::sharedTextureCache()->addImage(atlas.c_str(), false))
      s_font = fnt;
    else
      log::warn("Loading screen: the font's atlas {} doesn't load, GD's font instead", atlas);
    return *s_font;
  }

  std::string upper(std::string text)
  {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return std::toupper(c); });
    return text;
  }

  // A label in the mod's font by its file, else the gold font Geode itself uses here (loaded by now)
  CCLabelBMFont* label(std::string const& text, float cap, ccColor3B color)
  {
    auto font = fontFile();
    auto node = font.empty() ? nullptr : CCLabelBMFont::create(text.c_str(), font.c_str());
    if (!node)
      node = CCLabelBMFont::create(text.c_str(), "goldFont.fnt");
    if (!node)
      return nullptr;
    node->setScale(cap / FONT_CAP);
    node->setColor(color);
    return node;
  }

  // ! --- The intro --- !

  // Played once a session, at the game's start; meanwhile the loading waits (its steps block the game for a while
  // each, the video would stutter)
  bool s_introPlaying = false;
  bool s_introShown = false;

  // The intro: frames from resources/intro, one after another into the same texture, by the time its sound has
  // played (or the time that passed, without sound), over everything, fitted to the screen. A click or a key skips
  // it; at its end it fades into the loading screen
  class MSIntro : public CCLayer
  {
  public:
    static MSIntro* create()
    {
      auto intro = new MSIntro();
      if (intro->init())
      {
        intro->autorelease();
        return intro;
      }
      delete intro;
      return nullptr;
    }

    ~MSIntro() override { this->stopSound(); }

    bool init() override
    {
      if (!CCLayer::init())
        return false;
      auto const& theme = Theme::get();
      auto winSize = CCDirector::get()->getWinSize();
      // The frames there are: intro-0001.jpg on
      std::error_code error;
      auto dir = Mod::get()->getResourcesDir();
      while (std::filesystem::exists(dir / fmt::format("intro-{:04}.jpg", m_count + 1), error))
        m_count++;
      if (m_count == 0)
        return false;
      m_background = CCLayerColor::create(Theme::withAlpha(theme.introBackground, 255));
      m_background->setContentSize(winSize);
      this->addChild(m_background);
      this->startSound();
      this->setTouchEnabled(true);
      this->setKeyboardEnabled(true);
      this->showFrame(0);
      this->scheduleUpdate();
      return true;
    }

    void registerWithTouchDispatcher() override
    {
      // Over everything of the loading screen, every touch its own
      CCDirector::get()->getTouchDispatcher()->addTargetedDelegate(this, -1000, true);
    }

    bool ccTouchBegan(CCTouch*, CCEvent*) override
    {
      this->finish();
      return true;
    }

    void keyDown(enumKeyCodes, double) override { this->finish(); }

    void update(float dt) override
    {
      if (m_finished)
        return;
      m_time += std::min(dt, 0.1f);
      // The sound's clock when there is one: the frames keep with it
      if (m_channel)
      {
        unsigned position = 0;
        bool playing = false;
        if (m_channel->isPlaying(&playing) == FMOD_OK && playing &&
            m_channel->getPosition(&position, FMOD_TIMEUNIT_MS) == FMOD_OK)
          m_time = position / 1000.f;
      }
      auto index = static_cast<int>(m_time * std::max(Theme::get().introFps, 1.f));
      if (index >= m_count)
        return this->finish();
      if (index != m_shown)
        this->showFrame(index);
    }

  private:
    void showFrame(int index)
    {
      m_shown = index;
      auto path = (Mod::get()->getResourcesDir() / fmt::format("intro-{:04}.jpg", index + 1)).string();
      auto image = new CCImage();
      if (!image->initWithImageFile(path.c_str()))
      {
        image->release();
        return;
      }
      auto width = image->getWidth();
      auto height = image->getHeight();
      auto alpha = image->hasAlpha();
      if (!m_texture)
      {
        auto texture = new CCTexture2D();
        if (texture->initWithData(image->getData(), alpha ? kCCTexture2DPixelFormat_RGBA8888 : kCCTexture2DPixelFormat_RGB888,
                                  width, height, CCSize(width, height)))
        {
          texture->setAntiAliasTexParameters();
          m_texture = texture;
          m_width = width;
          m_height = height;
          // As big as fits the screen, its proportions kept; the rest is the intro's own background
          auto sprite = CCSprite::createWithTexture(texture);
          auto winSize = CCDirector::get()->getWinSize();
          auto size = sprite->getContentSize();
          sprite->setScale(std::min(winSize.width / size.width, winSize.height / size.height));
          sprite->setPosition(winSize / 2);
          this->addChild(sprite);
          m_sprite = sprite;
        }
        texture->release();
      }
      else if (width == m_width && height == m_height)
      {
        // The same texture filled again: nothing made or deleted every frame
        ccGLBindTexture2D(m_texture->getName());
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, alpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE,
                        image->getData());
      }
      image->release();
    }

    void startSound()
    {
      auto engine = FMODAudioEngine::get();
      auto system = engine ? engine->m_system : nullptr;
      std::error_code error;
      auto path = Mod::get()->getResourcesDir() / "intro.ogg";
      if (!system || !std::filesystem::exists(path, error))
        return;
      if (system->createSound(path.string().c_str(), FMOD_CREATESTREAM, nullptr, &m_sound) != FMOD_OK ||
          system->playSound(m_sound, nullptr, true, &m_channel) != FMOD_OK)
      {
        this->stopSound();
        return;
      }
      auto music = std::clamp(engine->m_musicVolume, 0.f, 1.f);
      m_channel->setVolume(std::clamp(Theme::get().introVolume, 0.f, 1.f) * music);
      m_channel->setPaused(false);
    }

    void stopSound()
    {
      if (m_channel)
        m_channel->stop();
      if (m_sound)
        m_sound->release();
      m_channel = nullptr;
      m_sound = nullptr;
    }

    // Done or skipped: the loading goes on, the intro fades into the loading screen
    void finish()
    {
      if (m_finished)
        return;
      m_finished = true;
      s_introPlaying = false;
      this->setTouchEnabled(false);
      this->setKeyboardEnabled(false);
      auto fade = std::max(Theme::get().introFadeOut, 0.f);
      if (m_channel)
        m_channel->setVolume(0);
      this->stopSound();
      for (auto node : std::initializer_list<CCNode*>{m_background, m_sprite})
      {
        if (auto rgba = typeinfo_cast<CCRGBAProtocol*>(node))
          static_cast<CCNode*>(node)->runAction(CCFadeTo::create(fade, 0));
      }
      this->runAction(CCSequence::createWithTwoActions(CCDelayTime::create(fade), CCRemoveSelf::create()));
    }

    int m_count = 0;
    int m_shown = -1;
    float m_time = 0;
    bool m_finished = false;
    CCLayerColor* m_background = nullptr;
    CCSprite* m_sprite = nullptr;
    Ref<CCTexture2D> m_texture;
    unsigned m_width = 0;
    unsigned m_height = 0;
    FMOD::Sound* m_sound = nullptr;
    FMOD::Channel* m_channel = nullptr;
  };
} // namespace

// MiSide's loading screen in place of GD's: a deep violet background lit magenta in the middle, a dark pattern of
// hearts and stars, "LOADING..." with a thin pink bar in the bottom left, GD's tip under it in the middle. GD's
// nodes hide; Geode's own lines (the mods it loads) stay, in this style. Needs the mod loaded early (mod.json)
class $modify(MSLoadingLayer, LoadingLayer)
{
  struct Fields
  {
    CCNode* m_fill = nullptr;
    CCNode* m_head = nullptr;
    CCLabelBMFont* m_dots = nullptr;
    float m_shown = 0;
    float m_time = 0;
    float m_barWidth = 0;
    float m_barHeight = 0;
  };

  static void onModify(auto& self)
  {
    // After Geode's own init, which adds its lines
    if (self.setHookPriorityPost("LoadingLayer::init", Priority::VeryLate).isErr())
      log::warn("Can't set the LoadingLayer::init hook's priority");
    // Before Geode's own steps, so the intro can hold all of them
    if (self.setHookPriorityPre("LoadingLayer::loadAssets", Priority::First).isErr())
      log::warn("Can't set the LoadingLayer::loadAssets hook's priority");
  }

  // While the intro plays, every step of the loading waits a frame and asks again
  void loadAssets()
  {
    if (s_introPlaying)
    {
      Loader::get()->queueInMainThread([layer = Ref(this)] { layer->loadAssets(); });
      return;
    }
    LoadingLayer::loadAssets();
  }

  bool init(bool refresh)
  {
    if (!LoadingLayer::init(refresh))
      return false;
    Theme::reload();
    auto vanilla = Mod::get()->getSettingValue<bool>("vanilla-menu");
    if (!vanilla && Theme::get().loadingEnabled)
      this->build();
    // The intro: at the game's start only (not when GD reloads its resources), once
    if (!refresh && !s_introShown && !vanilla && Theme::get().introEnabled &&
        Mod::get()->getSettingValue<bool>("show-intro"))
    {
      s_introShown = true;
      if (auto intro = MSIntro::create())
      {
        s_introPlaying = true;
        intro->setID("intro"_spr);
        this->addChild(intro, 10000);
      }
    }
    return true;
  }

  void build()
  {
    auto const& theme = Theme::get();
    auto winSize = CCDirector::get()->getWinSize();
    for (auto id : VANILLA)
    {
      if (auto node = this->getChildByID(id))
        node->setVisible(false);
    }

    // The background: deep violet, lit magenta from the middle, a dark pattern over it, the left kept clearer
    auto base = CCLayerColor::create(Theme::withAlpha(theme.loadingEdgeColor, 255));
    base->setContentSize(winSize);
    this->addChild(base, -10);
    if (auto glowPath = resource("glow-soft.png"); !glowPath.empty())
    {
      if (auto glow = CCSprite::create(glowPath.c_str()))
      {
        auto size = glow->getContentSize();
        if (size.width > 0 && size.height > 0)
        {
          glow->setScaleX(winSize.width * theme.loadingGlowSize / size.width);
          glow->setScaleY(winSize.height * theme.loadingGlowSize / size.height);
        }
        glow->setColor(theme.loadingGlowColor);
        glow->setOpacity(255);
        glow->setPosition({winSize.width * 0.52f, winSize.height * 0.52f});
        this->addChild(glow, -9);
      }
    }
    if (auto patternPath = resource("pattern-tile.png"); !patternPath.empty() && theme.loadingPatternOpacity > 0)
    {
      // Tiled by hand: a few sprites, no texture repeat needed
      auto probe = CCSprite::create(patternPath.c_str());
      auto tile = probe ? probe->getContentSize() : CCSize{};
      if (tile.width > 0 && tile.height > 0)
      {
        auto scale = theme.patternTile / tile.width;
        auto step = CCSize{tile.width * scale, tile.height * scale};
        for (float y = 0; y < winSize.height + step.height; y += step.height)
        {
          for (float x = 0; x < winSize.width + step.width; x += step.width)
          {
            auto piece = CCSprite::create(patternPath.c_str());
            piece->setScale(scale);
            piece->setAnchorPoint({0, 0});
            piece->setPosition({x, y});
            piece->setColor(theme.loadingPatternColor);
            piece->setOpacity(theme.loadingPatternOpacity);
            this->addChild(piece, -8);
          }
        }
      }
      // MiSide's pattern shows on the right: the left side covered back to the background
      auto clear = CCLayerGradient::create(Theme::withAlpha(theme.loadingEdgeColor, 230),
                                           Theme::withAlpha(theme.loadingEdgeColor, 0), {1, 0});
      clear->setContentSize({winSize.width * 0.6f, winSize.height});
      this->addChild(clear, -7);
    }

    // LOADING... and the bar under it, bottom left
    auto en = Mod::get()->getSettingValue<std::string>("menu-language") == "EN";
    auto left = theme.loadingLeft;
    if (auto caption = label(en ? "LOADING" : "ЗАГРУЗКА", theme.loadingCaptionCap, theme.loadingCaptionColor))
    {
      caption->setAnchorPoint({0, 0.5f});
      caption->setPosition({left, theme.loadingCaptionCapTop - theme.loadingCaptionCap / 2 -
                                    FONT_RAISE * caption->getScale()});
      this->addChild(caption, 5);
      if (auto dots = label("...", theme.loadingCaptionCap, theme.loadingCaptionColor))
      {
        dots->setAnchorPoint({0, 0.5f});
        dots->setPosition(caption->getPosition() + CCPoint{caption->getScaledContentSize().width, 0});
        this->addChild(dots, 5);
        m_fields->m_dots = dots;
      }
    }
    auto barWidth = theme.loadingBarWidth;
    auto barHeight = theme.loadingBarHeight;
    m_fields->m_barWidth = barWidth;
    m_fields->m_barHeight = barHeight;
    auto barY = theme.loadingBarY;
    auto track = CCLayerColor::create(Theme::withAlpha(theme.loadingTrackColor, 255), barWidth, barHeight);
    track->setPosition({left, barY});
    this->addChild(track, 5);
    // The track's light lower edge
    auto edge = CCLayerColor::create(Theme::withAlpha(theme.loadingTrackEdgeColor, 255), barWidth, 0.4f);
    edge->setPosition({left, barY - 0.4f});
    this->addChild(edge, 5);
    // The fill: dark violet at its start, lighter pink towards its head
    auto fill = CCLayerGradient::create(Theme::withAlpha(theme.loadingFillStartColor, 255),
                                        Theme::withAlpha(theme.loadingFillEndColor, 255), {1, 0});
    fill->setContentSize({0, barHeight});
    fill->setPosition({left, barY});
    this->addChild(fill, 6);
    m_fields->m_fill = fill;
    // Its bright head
    if (auto glowPath = resource("glow-soft.png"); !glowPath.empty())
    {
      if (auto head = CCSprite::create(glowPath.c_str()))
      {
        auto size = head->getContentSize();
        if (size.width > 0)
          head->setScale(barHeight * 3.2f / size.width);
        head->setColor(theme.loadingHeadColor);
        head->setBlendFunc({GL_SRC_ALPHA, GL_ONE});
        head->setPosition({left, barY + barHeight / 2});
        this->addChild(head, 7);
        m_fields->m_head = head;
      }
    }

    // GD's tip in MiSide's warning's place, bottom middle; Geode's lines under it
    if (auto tip = label(upper(this->getLoadingString()), theme.loadingTipCap, theme.loadingTipColor))
    {
      auto most = winSize.width - 24;
      if (tip->getScaledContentSize().width > most)
        tip->setScale(tip->getScale() * most / tip->getScaledContentSize().width);
      tip->setPosition({winSize.width / 2, theme.loadingTipY});
      this->addChild(tip, 5);
    }
    auto restyle = [&](std::string_view id, float y, ccColor3B color) {
      // Geode's own font kept (its label isn't ours to refont): only its colour, size and place
      auto line = typeinfo_cast<CCLabelBMFont*>(this->getChildByID(id));
      if (!line)
        return;
      line->setScale(0.3f);
      line->setColor(color);
      line->setPosition({winSize.width / 2, y});
      line->setZOrder(5);
    };
    restyle("geode-small-label", theme.loadingTipY - theme.loadingTipCap * 2.4f, theme.loadingCaptionColor);
    restyle("geode-small-label-2", theme.loadingTipY - theme.loadingTipCap * 4.6f, theme.loadingCaptionColor);

    this->schedule(schedule_selector(MSLoadingLayer::stepBar));
  }

  // The real progress is the vanilla bar's (Geode sets it at each step): the fill catches up softly
  void stepBar(float dt)
  {
    auto& f = *m_fields.self();
    auto progress = 0.f;
    if (m_sliderBar && m_sliderGrooveXPos > 0)
      progress = std::clamp(m_sliderBar->getTextureRect().size.width / m_sliderGrooveXPos, 0.f, 1.f);
    f.m_shown += (progress - f.m_shown) * std::min(1.f, dt * CATCH_UP);
    auto width = f.m_barWidth * f.m_shown;
    if (f.m_fill)
      f.m_fill->setContentSize({width, f.m_barHeight});
    if (f.m_head)
    {
      f.m_head->setPositionX(Theme::get().loadingLeft + width);
      f.m_head->setVisible(width > 0.5f);
    }
    // The dots counting up
    f.m_time += dt;
    if (f.m_dots)
    {
      static char const* const DOTS[] = {"", ".", "..", "..."};
      f.m_dots->setString(DOTS[static_cast<int>(f.m_time * 2.5f) % 4]);
    }
  }
};
