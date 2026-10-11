#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

// The decisions behind Mita's playback, with no cocos in them so they can be tested on their own: which frame the
// time that passed asks for, which atlas page to decode next, and which pages to free so that at most three are
// ever on the GPU (the one for now and the next two: a page holds only 83 ms of her). A page further ahead is only
// decoded, and waits on the CPU
class MitaPlan
{
public:
  // What to do with a page that finished decoding
  enum class Use
  {
    // The page for now or one of the next two: on the GPU with it
    Upload,
    // Needed soon, not yet: it waits decoded until it's one of the next two
    Wait,
    // Behind, or on the GPU already
    Drop,
  };

  // Pages on the GPU at most: the one for now and the next two
  static constexpr int RESIDENT = 3;
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

  // The page for now or one of the next two (around the loop: the last page's next is the first)
  bool inWindow(int atlas, int wanted) const { return this->distance(wanted, atlas) < RESIDENT; }

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
    // On the GPU only when there's room without the page shown or the ones for now and next
    if (this->inWindow(decoded, wanted) && this->hasRoom(wanted))
      return Use::Upload;
    return Use::Wait;
  }

  // A page can go on the GPU: fewer than three there, or one that's neither shown nor for now or next
  bool hasRoom(int wanted) const
  {
    if (static_cast<int>(m_resident.size()) < RESIDENT)
      return true;
    return std::any_of(m_resident.begin(), m_resident.end(),
                       [&](int atlas) { return this->isSpare(atlas, wanted); });
  }

  // What to ask the decoder for, nearest first: the page for now and the next two that aren't on the GPU, and the
  // one after them to wait decoded. `predicted` is the page playback will be on once a decode ends (from how long
  // they take): the same as `wanted` with a fast decoder, further on with a slow one, and then the pages before it
  // would come too late and aren't asked for
  std::vector<int> toRequest(int wanted, int predicted) const
  {
    if (this->distance(wanted, predicted) > LOOKAHEAD)
      predicted = wanted;
    // From the first page that wouldn't come too late: four pages, never further than a page may wait
    auto first = this->distance(wanted, predicted);
    std::vector<int> pages;
    for (int ahead = first; ahead <= std::min(first + RESIDENT, LOOKAHEAD); ahead++)
    {
      auto page = (wanted + ahead) % m_atlasCount;
      if (!this->isResident(page) && std::find(pages.begin(), pages.end(), page) == pages.end())
        pages.push_back(page);
    }
    return pages;
  }

  // Before an upload: the pages to free so the new one makes at most three. Never the one shown, or the ones for
  // now and next
  std::vector<int> makeRoom(int wanted)
  {
    std::vector<int> freed;
    while (static_cast<int>(m_resident.size()) >= RESIDENT)
    {
      auto it = std::find_if(m_resident.begin(), m_resident.end(),
                             [&](int atlas) { return this->isSpare(atlas, wanted); });
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

  // After a frame of this page is shown (the sprite on it already): the pages to free, all but it and the next two
  std::vector<int> showed(int atlas)
  {
    m_shown = atlas;
    std::vector<int> freed;
    std::erase_if(m_resident, [&](int each) {
      if (this->inWindow(each, atlas))
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
  // On the GPU but neither shown nor for now or next: free to go
  bool isSpare(int atlas, int wanted) const { return atlas != m_shown && !this->inWindow(atlas, wanted); }

  int m_atlasCount;
  std::vector<int> m_resident;
  int m_shown = -1;
};
