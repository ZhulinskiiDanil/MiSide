#include "MSPauseAmbient.hpp"

#include "../Music.hpp"
#include "../Theme.hpp"

#include <Geode/fmod/fmod.hpp>

#include <set>

using namespace geode::prelude;

namespace
{
  void logOnce(std::string const& message)
  {
    static std::set<std::string> s_logged;
    if (s_logged.insert(message).second)
      log::info("Pause ambient: {}", message);
  }
} // namespace

MSPauseAmbient* MSPauseAmbient::get()
{
  static auto s_ambient = [] {
    auto ambient = new MSPauseAmbient();
    return ambient;
  }();
  return s_ambient;
}

void MSPauseAmbient::start(std::string const& name, float volume)
{
  if (name.empty())
    return;
  // Not found: logged once by find()
  auto path = music::find(name);
  if (path.empty())
    return;
  // Another ambient than the one fading out: that one goes at once
  if (m_channel && path != m_path)
    this->release();
  m_volume = std::clamp(volume, 0.f, 1.f);
  // Still playing (fading out): back in, the same one
  if (!m_channel)
  {
    auto system = FMODAudioEngine::get()->m_system;
    if (!system ||
        system->createSound(path.c_str(), FMOD_LOOP_NORMAL | FMOD_CREATESTREAM, nullptr, &m_sound) != FMOD_OK ||
        system->playSound(m_sound, nullptr, true, &m_channel) != FMOD_OK)
    {
      logOnce(fmt::format("{} can't be played", path));
      this->release();
      return;
    }
    m_path = path;
    m_level = 0;
    m_channel->setVolume(0);
    m_channel->setPaused(false);
  }
  m_target = 1;
  this->schedule();
}

void MSPauseAmbient::playOnce(std::string const& name, float volume)
{
  if (name.empty())
    return;
  this->releaseOnce();
  auto path = std::string(CCFileUtils::get()->fullPathForFilename(name.c_str(), false));
  auto engine = FMODAudioEngine::get();
  auto system = engine->m_system;
  if (path.empty() || !system ||
      system->createSound(path.c_str(), FMOD_DEFAULT, nullptr, &m_onceSound) != FMOD_OK ||
      system->playSound(m_onceSound, nullptr, true, &m_onceChannel) != FMOD_OK)
  {
    logOnce(fmt::format("{} can't be played", name));
    this->releaseOnce();
    return;
  }
  auto level = std::clamp(volume, 0.f, 2.f) * std::clamp(Theme::get().soundVolume, 0.f, 1.f) *
               std::clamp(engine->m_sfxVolume, 0.f, 1.f);
  m_onceChannel->setVolume(level);
  m_onceChannel->setPaused(false);
  this->schedule();
}

void MSPauseAmbient::stop()
{
  m_target = 0;
  this->releaseOnce();
}

void MSPauseAmbient::schedule()
{
  if (m_scheduled)
    return;
  CCDirector::get()->getScheduler()->scheduleUpdateForTarget(this, 0, false);
  m_scheduled = true;
}

void MSPauseAmbient::releaseOnce()
{
  if (m_onceChannel)
    m_onceChannel->stop();
  if (m_onceSound)
    m_onceSound->release();
  m_onceChannel = nullptr;
  m_onceSound = nullptr;
}

void MSPauseAmbient::update(float dt)
{
  auto const& theme = Theme::get();
  auto fade = std::max(theme.pauseAmbientFade, 0.01f);
  auto step = dt / fade;
  m_level = m_target > m_level ? std::min(m_level + step, m_target) : std::max(m_level - step, m_target);
  // The one-shot, once it's done
  if (m_onceChannel)
  {
    bool playing = false;
    if (m_onceChannel->isPlaying(&playing) != FMOD_OK || !playing)
      this->releaseOnce();
  }
  if (m_channel)
  {
    bool playing = false;
    if (m_channel->isPlaying(&playing) != FMOD_OK || !playing)
      this->release();
    else
    {
      auto music = FMODAudioEngine::get()->m_musicVolume;
      m_channel->setVolume(m_volume * std::clamp(music, 0.f, 1.f) * m_level);
    }
  }
  if (m_channel && m_level <= 0 && m_target <= 0)
    this->release();
  if (!m_channel && !m_onceChannel && m_scheduled)
  {
    CCDirector::get()->getScheduler()->unscheduleUpdateForTarget(this);
    m_scheduled = false;
  }
}

void MSPauseAmbient::release()
{
  // Only its own channel and sound
  if (m_channel)
    m_channel->stop();
  if (m_sound)
    m_sound->release();
  m_channel = nullptr;
  m_sound = nullptr;
  m_path.clear();
  m_level = 0;
}
