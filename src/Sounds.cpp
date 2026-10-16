#include "Sounds.hpp"

#include "Theme.hpp"
#include "ui/MSStyle.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/CCMenuItemToggler.hpp>
#include <Geode/modify/FMODAudioEngine.hpp>
#include <Geode/modify/GJGarageLayer.hpp>
#include <Geode/modify/MoreOptionsLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <array>

using namespace geode::prelude;

namespace
{
  // GD's own sounds the mod's take the place of, so the moment sounds once
  constexpr char const* GD_ACHIEVEMENT = "achievement_01.ogg";
  constexpr char const* GD_LEVEL_END = "endStart_02.ogg";

  // The icon kit's icons, to tell a new one was put on: every kind of icon GD keeps
  constexpr std::array<IconType, 12> ICON_TYPES = {
    IconType::Cube,   IconType::Ship,   IconType::Ball,  IconType::Ufo,         IconType::Wave,
    IconType::Robot,  IconType::Spider, IconType::Swing, IconType::Jetpack,     IconType::DeathEffect,
    IconType::Special, IconType::ShipFire,
  };

  std::array<int, ICON_TYPES.size()> icons()
  {
    std::array<int, ICON_TYPES.size()> active{};
    auto manager = GameManager::get();
    for (size_t i = 0; i < ICON_TYPES.size(); i++)
      active[i] = manager->activeIconForType(ICON_TYPES[i]);
    return active;
  }

  // A toggle in GD's settings (the main ones, the pause's, More Options)
  bool inSettings(CCNode* node)
  {
    for (auto parent = node ? node->getParent() : nullptr; parent; parent = parent->getParent())
    {
      if (typeinfo_cast<MoreOptionsLayer*>(parent) || typeinfo_cast<GJOptionsLayer*>(parent) ||
          typeinfo_cast<OptionsLayer*>(parent))
        return true;
    }
    return false;
  }
} // namespace

// GD's achievement and level end sounds, the mod's in their place (with the same volume GD asked for)
class $modify(MSSoundEngine, FMODAudioEngine)
{
  int playEffectAdvanced(gd::string path, float speed, float unknown, float volume, float pitch, bool fft, bool reverb,
                         int startMillis, int endMillis, int fadeIn, int fadeOut, bool loop, int effectID, bool override,
                         bool noPreload, int channelID, int uniqueID, float minInterval, int sfxGroup)
  {
    auto const& theme = Theme::get();
    std::string file = path;
    std::string const* replacement = nullptr;
    if (file == GD_ACHIEVEMENT)
      replacement = &theme.achievementSound;
    else if (file == GD_LEVEL_END)
      replacement = &theme.winSound;
    if (replacement && !replacement->empty() && ms::resourceExists(*replacement))
      path = *replacement;
    return FMODAudioEngine::playEffectAdvanced(path, speed, unknown, volume, pitch, fft, reverb, startMillis, endMillis,
                                               fadeIn, fadeOut, loop, effectID, override, noPreload, channelID, uniqueID,
                                               minInterval, sfxGroup);
  }
};

// Table Toggle for GD's settings toggles
class $modify(MSSoundToggler, CCMenuItemToggler)
{
  void activate()
  {
    CCMenuItemToggler::activate();
    if (inSettings(this))
      ms::playSound(Theme::get().toggleSound);
  }
};

// Switch Kategory for More Options' pages
// GD 2.209 reworked this screen; the hooked functions are gone
#if GEODE_COMP_GD_VERSION < 22090
class $modify(MSSoundMoreOptions, MoreOptionsLayer)
{
  void onNextPage(CCObject* sender)
  {
    MoreOptionsLayer::onNextPage(sender);
    ms::playSound(Theme::get().tabSound);
  }

  void onPrevPage(CCObject* sender)
  {
    MoreOptionsLayer::onPrevPage(sender);
    ms::playSound(Theme::get().tabSound);
  }
};
#endif


// The icon kit: Switch Kategory for its tabs, Gift Poof when an icon is put on (not for a locked one)
class $modify(MSSoundGarage, GJGarageLayer)
{
  void onSelectTab(CCObject* sender)
  {
    GJGarageLayer::onSelectTab(sender);
    ms::playSound(Theme::get().tabSound);
  }

  void onSelect(CCObject* sender)
  {
    auto before = icons();
    GJGarageLayer::onSelect(sender);
    if (icons() != before)
      ms::playSound(Theme::get().outfitSound);
  }
};

// The personal best as the attempt started: GD raises it only once the attempt ends
class $modify(MSSoundPlayLayer, PlayLayer)
{
  struct Fields
  {
    int m_attemptBest = 0;
  };

  bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects)
  {
    if (level)
      m_fields->m_attemptBest = level->m_normalPercent.value();
    return PlayLayer::init(level, useReplay, dontCreateObjects);
  }

  void resetLevel()
  {
    if (m_level)
      m_fields->m_attemptBest = m_level->m_normalPercent.value();
    PlayLayer::resetLevel();
  }
};

namespace sounds
{
  float stressLevel()
  {
    auto const& theme = Theme::get();
    auto level = static_cast<MSSoundPlayLayer*>(PlayLayer::get());
    if (!level || !level->m_level || level->m_isPracticeMode || level->m_isTestMode || level->m_level->isPlatformer())
      return 0;
    auto best = level->m_fields->m_attemptBest;
    // A completed level, or a best too low to be near
    if (best >= 100 || best < theme.pauseStressMinBest)
      return 0;
    auto now = level->getCurrentPercent();
    if (now > best)
      return std::max(theme.pauseStressBoost, 1.f);
    return now >= best - std::max(theme.pauseStressWindow, 0.f) ? 1.f : 0.f;
  }
} // namespace sounds
