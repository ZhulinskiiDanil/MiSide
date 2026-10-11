#pragma once

#include <algorithm>

// One pass of the shine through an item's letters, without cocos so it can be tested on its own. It moves by the
// time that passed (the same at any frame rate); the band's middle goes from fully past the label's left edge to
// fully past its right one, in the label's own width (0: its left edge, 1: its right edge)
class ShinePass
{
public:
  // A new pass from the start (hovering the item again restarts it)
  void start()
  {
    m_elapsed = 0;
    m_fade = 1;
    m_leaving = false;
    m_active = true;
  }

  // The mouse left: it fades out over `fadeOut` seconds instead of stopping short
  void leave()
  {
    if (m_active)
      m_leaving = true;
  }

  void advance(float dt, float duration, float fadeOut)
  {
    if (!m_active)
      return;
    m_elapsed += std::max(dt, 0.f);
    if (m_leaving)
      m_fade = fadeOut > 0 ? std::max(0.f, m_fade - std::max(dt, 0.f) / fadeOut) : 0.f;
    if (m_elapsed >= duration || m_fade <= 0)
      m_active = false;
  }

  bool active() const { return m_active; }
  float fade() const { return m_fade; }

  float progress(float duration) const { return duration > 0 ? std::clamp(m_elapsed / duration, 0.f, 1.f) : 1.f; }

  // Where the band's middle is, in label widths: -width/2 (hidden on the left) to 1 + width/2 (gone on the right)
  float center(float duration, float width) const { return -width / 2 + this->progress(duration) * (1 + width); }

private:
  float m_elapsed = 0;
  float m_fade = 1;
  bool m_leaving = false;
  bool m_active = false;
};
