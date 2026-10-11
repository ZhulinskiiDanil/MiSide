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

void MSPauseAmbient::start()
{
  auto const& theme = Theme::get();
  if (!theme.pauseAmbientEnabled)
    return;
  if (theme.pauseAmbientPath.empty())
  {
    logOnce("on, but no file is set (pause-ambient-path)");
    return;
  }
  // Still playing (fading out): back in, the same one
  if (!m_channel)
  {
    // Not found: logged once by find()
    auto path = music::find(theme.pauseAmbientPath);
    if (path.empty())
      return;
    auto system = FMODAudioEngine::get()->m_system;
    if (!system ||
        system->createSound(path.c_str(), FMOD_LOOP_NORMAL | FMOD_CREATESTREAM, nullptr, &m_sound) != FMOD_OK ||
        system->playSound(m_sound, nullptr, true, &m_channel) != FMOD_OK)
    {
      logOnce(fmt::format("{} can't be played", path));
      this->release();
      return;
    }
    m_level = 0;
    m_channel->setVolume(0);
    m_channel->setPaused(false);
  }
  m_target = 1;
  if (!m_scheduled)
  {
    CCDirector::get()->getScheduler()->scheduleUpdateForTarget(this, 0, false);
    m_scheduled = true;
  }
}

void MSPauseAmbient::stop() { m_target = 0; }

void MSPauseAmbient::update(float dt)
{
  auto const& theme = Theme::get();
  auto fade = std::max(theme.pauseAmbientFade, 0.01f);
  auto step = dt / fade;
  m_level = m_target > m_level ? std::min(m_level + step, m_target) : std::max(m_level - step, m_target);
  if (m_channel)
  {
    bool playing = false;
    if (m_channel->isPlaying(&playing) != FMOD_OK || !playing)
    {
      this->release();
      return;
    }
    auto music = FMODAudioEngine::get()->m_musicVolume;
    m_channel->setVolume(std::clamp(theme.pauseAmbientVolume, 0.f, 1.f) * std::clamp(music, 0.f, 1.f) * m_level);
  }
  if (m_level <= 0 && m_target <= 0)
    this->release();
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
  m_level = 0;
  if (m_scheduled)
  {
    CCDirector::get()->getScheduler()->unscheduleUpdateForTarget(this);
    m_scheduled = false;
  }
}
