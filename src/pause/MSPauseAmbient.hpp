#pragma once

#include <Geode/Geode.hpp>

namespace FMOD
{
  class Sound;
  class Channel;
} // namespace FMOD

// The pause's own ambient: one looping stream on a channel of its own (never the level's song, never stopAll),
// faded in when the pause opens and out when it closes. Its volume is the theme's times GD's music volume, so muted
// music mutes it. Opened again while it fades out, it fades back in: never two. Runs on cocos' scheduler, not on a
// node, so it can finish fading out after the pause menu is gone
class MSPauseAmbient : public cocos2d::CCObject
{
public:
  static MSPauseAmbient* get();

  // Off in the theme, or no file: nothing (the missing file logged once)
  void start();
  void stop();
  void update(float dt) override;

protected:
  void release();

  FMOD::Sound* m_sound = nullptr;
  FMOD::Channel* m_channel = nullptr;
  // 0..1 of the fade, and where it's going
  float m_level = 0;
  float m_target = 0;
  bool m_scheduled = false;
};
