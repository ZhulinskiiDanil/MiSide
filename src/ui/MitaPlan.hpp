#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

// The decisions behind Mita's playback, with no cocos in them so they can be tested on their own: which frame the
// time that passed asks for, which atlas page to decode next, and which pages to free so that at most two are ever
// on the GPU (the one shown and the next). Pages further ahead are only decoded, and wait on the CPU
class MitaPlan
{
public:
  // What to do with a page that finished decoding
  enum class Use
  {
    // The page for now or the next one: on the GPU with it
    Upload,
    // Needed soon, not yet: it waits decoded until it's the next one
    Wait,
    // Behind, or on the GPU already
    Drop,
  };

  // How many pages ahead a decoded page may wait for its turn
  static constexpr int LOOKAHEAD = 4;

  explicit MitaPlan(int atlasCount) : m_atlasCount(std::max(atlasCount, 1)) {}

  // The frame for this moment of the loop
  static int frameAt(double elapsed, double fps, int frameCount)
  {
    if (frameCount <= 0 || fps <= 0)
      return 0;
    auto index = static_cast<long long>(std::floor(elapsed * fps)) % frameCount;
    return static_cast<int>(index < 0 ? index + frameCount : index);
  }

  // The time in the loop after dt more seconds; it wraps, so it never grows big enough to lose precision
  static double advance(double elapsed, double dt, double fps, int frameCount)
  {
    if (frameCount <= 0 || fps <= 0)
      return 0;
    auto duration = frameCount / fps;
    auto result = std::fmod(elapsed + std::max(dt, 0.0), duration);
    return result < 0 ? result + duration : result;
  }

  int next(int atlas) const { return (atlas + 1) % m_atlasCount; }

  // Pages forward from one to the other, around the loop
  int distance(int from, int to) const { return ((to - from) % m_atlasCount + m_atlasCount) % m_atlasCount; }

  bool isResident(int atlas) const { return std::find(m_resident.begin(), m_resident.end(), atlas) != m_resident.end(); }

  int shown() const { return m_shown; }
  std::vector<int> const& resident() const { return m_resident; }

  Use decide(int decoded, int wanted) const
  {
    if (this->isResident(decoded))
      return Use::Drop;
    auto ahead = this->distance(wanted, decoded);
    if (ahead > LOOKAHEAD)
      return Use::Drop;
    // On the GPU only when there's room without the page shown or the one for now
    if (ahead <= 1 && this->hasRoom(wanted))
      return Use::Upload;
    return Use::Wait;
  }

  // A page can go on the GPU: fewer than two there, or one that's neither shown nor wanted to free
  bool hasRoom(int wanted) const
  {
    if (m_resident.size() < 2)
      return true;
    return std::any_of(m_resident.begin(), m_resident.end(),
                       [&](int atlas) { return atlas != m_shown && atlas != wanted; });
  }

  // What to ask the decoder for now. `predicted` is the page playback will be on once a decode ends (from how long
  // they take): the same as `wanted` with a fast decoder, further on with a slow one. -1: nothing
  int toRequest(int wanted, int predicted) const
  {
    if (this->distance(wanted, predicted) > LOOKAHEAD)
      predicted = wanted;
    // The page for now is missing: the one it will be by the time it's ready
    if (!this->isResident(wanted))
      return this->isResident(predicted) ? -1 : predicted;
    auto following = this->next(wanted);
    if (!this->isResident(following))
    {
      // A decoder slower than a page: the next one would come too late
      if (this->distance(wanted, predicted) >= 2 && !this->isResident(predicted))
        return predicted;
      return following;
    }
    // Both on the GPU: the one after waits decoded on the CPU (the last page holds one frame only)
    auto after = this->next(following);
    return this->isResident(after) ? -1 : after;
  }

  // Before an upload: the pages to free so the new one makes at most two. Never the one shown or the one for now
  std::vector<int> makeRoom(int wanted)
  {
    std::vector<int> freed;
    while (m_resident.size() >= 2)
    {
      auto it = std::find_if(m_resident.begin(), m_resident.end(),
                             [&](int atlas) { return atlas != m_shown && atlas != wanted; });
      if (it == m_resident.end())
        break;
      freed.push_back(*it);
      m_resident.erase(it);
    }
    return freed;
  }

  void uploaded(int atlas)
  {
    if (!this->isResident(atlas))
      m_resident.push_back(atlas);
  }

  // After a frame of this page is shown: the pages to free, all but it and the next
  std::vector<int> showed(int atlas)
  {
    m_shown = atlas;
    std::vector<int> freed;
    auto keepNext = this->next(atlas);
    std::erase_if(m_resident, [&](int each) {
      if (each == atlas || each == keepNext)
        return false;
      freed.push_back(each);
      return true;
    });
    return freed;
  }

  // Everything freed (leaving the menu)
  std::vector<int> clear()
  {
    auto freed = m_resident;
    m_resident.clear();
    m_shown = -1;
    return freed;
  }

private:
  int m_atlasCount;
  std::vector<int> m_resident;
  int m_shown = -1;
};
