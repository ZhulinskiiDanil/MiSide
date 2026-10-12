#include "Music.hpp"

#include "Theme.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/FMODAudioEngine.hpp>
#include <Geode/modify/GJGarageLayer.hpp>
#include <Geode/modify/GameManager.hpp>

#include <filesystem>
#include <set>

using namespace geode::prelude;

namespace
{
  constexpr int MENU_CHANNEL = 0;

  void logOnce(std::string const& message)
  {
    static std::set<std::string> s_logged;
    if (s_logged.insert(message).second)
      log::warn("Music: {}", message);
  }

  bool replaced() { return !Mod::get()->getSettingValue<bool>("vanilla-menu"); }
} // namespace

namespace music
{
  std::string find(std::string const& name)
  {
    if (name.empty())
      return {};
    std::error_code error;
    std::filesystem::path path(name);
    if (path.is_absolute())
      return std::filesystem::exists(path, error) ? string::pathToString(path) : std::string();
    for (auto const& dir : {Mod::get()->getConfigDir(), Mod::get()->getResourcesDir()})
    {
      auto candidate = dir / path;
      if (std::filesystem::exists(candidate, error))
        return string::pathToString(candidate);
    }
    logOnce(fmt::format("{} not found (a path, or a name in the mod's config folder or resources)", name));
    return {};
  }

  std::string menuTrack()
  {
    auto const& theme = Theme::get();
    return theme.menuMusicEnabled && replaced() ? find(theme.menuMusicPath) : std::string();
  }

  std::string garageTrack()
  {
    auto const& theme = Theme::get();
    return theme.garageMusicEnabled && replaced() ? find(theme.garageMusicPath) : std::string();
  }
} // namespace music

// The theme before GD first asks for the menu's music (the loading screen's end, before MenuLayer)
$on_mod(Loaded) { Theme::reload(); }

// The menu's track in place of GD's menu loop: GD plays it, keeps it going through the menus without starting it
// again, stops it for a level and follows the music volume and mute as for its own
class $modify(MSMusicGameManager, GameManager)
{
  gd::string getMenuMusicFile()
  {
    auto track = music::menuTrack();
    return track.empty() ? GameManager::getMenuMusicFile() : gd::string(track);
  }
};

// Each of the mod's tracks at its own volume, relative to the music volume. GD fades music with FMOD's fade points,
// which come on top of this, so its fades still go from silence to here
class $modify(MSMusicEngine, FMODAudioEngine)
{
  void loadMusic(gd::string path, float speed, float unknown, float volume, bool shouldLoop, int musicID, int channelID,
                 bool dontReset)
  {
    auto const& theme = Theme::get();
    std::string file = path;
    if (!file.empty())
    {
      if (file == music::menuTrack())
        volume *= std::clamp(theme.menuMusicVolume, 0.f, 1.f);
      else if (file == music::garageTrack())
        volume *= std::clamp(theme.garageMusicVolume, 0.f, 1.f);
    }
    FMODAudioEngine::loadMusic(path, speed, unknown, volume, shouldLoop, musicID, channelID, dontReset);
  }
};

// The icon kit's own track: the menu's fades out, then it fades in; on leaving it fades out, and the main menu
// brings its own back
class $modify(MSMusicGarage, GJGarageLayer)
{
  bool init()
  {
    if (!GJGarageLayer::init())
      return false;
    auto track = music::garageTrack();
    auto engine = FMODAudioEngine::get();
    // Already playing (back from a popup that made the layer again): nothing to do
    if (track.empty() || engine->isMusicPlaying(track, MENU_CHANNEL))
      return true;
    auto fade = std::clamp(Theme::get().garageMusicFade, 0.f, 3.f);
    engine->fadeOutMusic(fade, MENU_CHANNEL);
    // On the layer: gone with it if it's left before then
    this->runAction(CCSequence::createWithTwoActions(
      CCDelayTime::create(fade), CCCallFunc::create(this, callfunc_selector(MSMusicGarage::startTrack))));
    return true;
  }

  void startTrack()
  {
    auto track = music::garageTrack();
    if (!track.empty())
      FMODAudioEngine::get()->playMusic(track, true, std::clamp(Theme::get().garageMusicFade, 0.f, 3.f), MENU_CHANNEL);
  }

  void onBack(CCObject* sender)
  {
    auto track = music::garageTrack();
    if (!track.empty() && FMODAudioEngine::get()->isMusicPlaying(track, MENU_CHANNEL))
      FMODAudioEngine::get()->fadeOutMusic(std::clamp(Theme::get().garageMusicFade, 0.f, 3.f), MENU_CHANNEL);
    GJGarageLayer::onBack(sender);
  }
};
