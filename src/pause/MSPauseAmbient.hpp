#pragma once

#include <Geode/Geode.hpp>

namespace FMOD
{
  class Sound;
  class Channel;
} // namespace FMOD

// The pause's own sounds, each on a channel of its own (never the level's song, never stopAll): a looping ambient,
// faded in when the pause opens and out when it closes, and a one-shot sound as it opens, cut when it closes. The
// ambient's volume is its own times GD's music volume (muted music mutes it), the one-shot's times the SFX volume.
// Opened again while the ambient fades out, the same one fades back in: never two. Runs on cocos' scheduler, not on
// a node, so it can finish fading out after the pause menu is gone
class MSPauseAmbient : public cocos2d::CCObject
{
public:
  static MSPauseAmbient* get();

  // name: a file name in the mod's resources or config folder, or a path. Another file than the one playing: that
  // one goes and this one fades in
  void start(std::string const& name, float volume);
  // A sound (cocos' name for it) once, at `volume` times the theme's sound volume and GD's SFX volume
  void playOnce(std::string const& name, float volume);
  // The ambient fades out, the one-shot stops
  void stop();
  void update(float dt) override;

protected:
  void release();
  void releaseOnce();
  void schedule();

  FMOD::Sound* m_sound = nullptr;
  FMOD::Channel* m_channel = nullptr;
  std::string m_path;
  float m_volume = 0;
  FMOD::Sound* m_onceSound = nullptr;
  FMOD::Channel* m_onceChannel = nullptr;
  // 0..1 of the fade, and where it's going
  float m_level = 0;
  float m_target = 0;
  bool m_scheduled = false;
};
