#include "Theme.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/LoadingLayer.hpp>

#include <filesystem>

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
  // its full path, in the texture quality's variant when there is one
  std::string resource(std::string const& name)
  {
    std::error_code error;
    auto dir = Mod::get()->getResourcesDir();
    std::filesystem::path file(name);
    auto factor = CC_CONTENT_SCALE_FACTOR();
    auto suffix = factor >= 4 ? "-uhd" : factor >= 2 ? "-hd" : "";
    auto variant = dir / (file.stem().string() + suffix + file.extension().string());
    if (*suffix && std::filesystem::exists(variant, error))
      return variant.string();
    auto plain = dir / file;
    return std::filesystem::exists(plain, error) ? plain.string() : std::string();
  }

  std::string upper(std::string text)
  {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return std::toupper(c); });
    return text;
  }

  // A label in the mod's font by its file, else GD's chat font; its capitals' top at capTop
  CCLabelBMFont* label(std::string const& text, float cap, ccColor3B color)
  {
    auto font = resource("miside-font.fnt");
    auto node = font.empty() ? nullptr : CCLabelBMFont::create(text.c_str(), font.c_str());
    if (!node)
      node = CCLabelBMFont::create(text.c_str(), "chatFont.fnt");
    if (!node)
      return nullptr;
    node->setScale(cap / FONT_CAP);
    node->setColor(color);
    return node;
  }
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
  }

  bool init(bool refresh)
  {
    if (!LoadingLayer::init(refresh))
      return false;
    Theme::reload();
    if (Mod::get()->getSettingValue<bool>("vanilla-menu") || !Theme::get().loadingEnabled)
      return true;
    this->build();
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
      auto line = typeinfo_cast<CCLabelBMFont*>(this->getChildByID(id));
      if (!line)
        return;
      if (auto font = resource("miside-font.fnt"); !font.empty())
        line->setFntFile(font.c_str());
      line->setScale(theme.loadingTipCap / FONT_CAP);
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
