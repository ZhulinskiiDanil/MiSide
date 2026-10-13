#include "MSWave.hpp"

#include "../Theme.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

#ifdef GEODE_IS_WINDOWS
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

using namespace geode::prelude;

namespace
{
  constexpr int WAVE_ACTION = 0x4d70;
  // Set on a node a wave hid, so the next wave in shows it (and only those of the hidden ones)
  constexpr auto HIDDEN_KEY = "wave-hidden"_spr;

  // A node of GD's own classes (its vtable in GD's code): its setOpacity can't be called on any node (GJItemIcon's
  // reads members some icons never set), so it only follows its parent's through cocos' cascade
  bool isGames(CCNode* node)
  {
    uintptr_t vtable = 0;
    std::memcpy(&vtable, static_cast<void const*>(node), sizeof(vtable));
    auto base = geode::base::get();
#ifdef GEODE_IS_WINDOWS
    static uintptr_t const SIZE = [base] {
      auto dos = reinterpret_cast<IMAGE_DOS_HEADER const*>(base);
      auto nt = reinterpret_cast<IMAGE_NT_HEADERS const*>(base + dos->e_lfanew);
      return static_cast<uintptr_t>(nt->OptionalHeader.SizeOfImage);
    }();
    return vtable >= base && vtable < base + SIZE;
#else
    Dl_info info{};
    return dladdr(reinterpret_cast<void*>(vtable), &info) && info.dli_fbase == reinterpret_cast<void*>(base);
#endif
  }

  // How a node's look fades: the topmost nodes with an opacity get theirs set (each with its own), everything under
  // them follows through cocos' cascade, turned on for the wave where it was off
  struct Fade
  {
    std::vector<std::pair<Ref<CCNode>, GLubyte>> tops;
    std::vector<Ref<CCNode>> cascaded;
  };

  void collect(CCNode* node, bool followsParent, Fade& fade)
  {
    auto rgba = typeinfo_cast<CCRGBAProtocol*>(node);
    if (rgba)
    {
      // Under no opacity of ours a node of GD's can't fade (it's left as it is)
      if (!followsParent && !isGames(node))
        fade.tops.emplace_back(node, rgba->getOpacity());
      if (!rgba->isCascadeOpacityEnabled())
      {
        rgba->setCascadeOpacityEnabled(true);
        fade.cascaded.emplace_back(node);
      }
    }
    // Its children follow when its own opacity moves: set here, or following its parent's
    auto driven = rgba && (followsParent || !isGames(node));
    for (auto child : CCArrayExt<CCNode*>(node->getChildren()))
      collect(child, driven, fade);
  }

  // Back as it was: the tops' opacities, then the cascade off again where it was, what's under those shown with
  // their own opacities again
  void restore(Fade const& fade)
  {
    for (auto const& [node, opacity] : fade.tops)
    {
      if (auto rgba = typeinfo_cast<CCRGBAProtocol*>(node.data()))
        rgba->setOpacity(opacity);
    }
    for (auto const& node : fade.cascaded)
    {
      if (auto rgba = typeinfo_cast<CCRGBAProtocol*>(node.data()))
        rgba->setCascadeOpacityEnabled(false);
      for (auto child : CCArrayExt<CCNode*>(node->getChildren()))
      {
        if (auto inner = typeinfo_cast<CCRGBAProtocol*>(child))
          inner->updateDisplayedOpacity(255);
      }
    }
  }

  // One node of a step: from its place (and opacity) to the right and transparent, or from the left into them
  class WaveAction final : public CCActionInterval
  {
  public:
    static WaveAction* create(bool in, float delay, float time, float slide)
    {
      auto action = new WaveAction();
      action->m_in = in;
      action->m_delay = std::max(delay, 0.f);
      action->m_time = std::max(time, 0.01f);
      action->m_slide = slide;
      action->initWithDuration(action->m_delay + action->m_time);
      action->autorelease();
      return action;
    }

    void startWithTarget(CCNode* target) override
    {
      CCActionInterval::startWithTarget(target);
      m_home = target->getPosition();
      collect(target, false, m_fade);
      // Coming in: out of sight from the first frame, through its delay
      this->apply(0);
    }

    void update(float t) override
    {
      auto elapsed = t * this->getDuration();
      this->apply(std::clamp((elapsed - m_delay) / m_time, 0.f, 1.f));
    }

    void stop() override
    {
      this->settle();
      CCActionInterval::stop();
    }

    // Where it ends, at once: in its place, as opaque as it was; hidden if it went out
    void settle()
    {
      if (m_settled || !m_pTarget)
        return;
      m_settled = true;
      m_pTarget->setPosition(m_home);
      restore(m_fade);
      if (!m_in)
      {
        m_pTarget->setVisible(false);
        m_pTarget->setUserObject(HIDDEN_KEY, CCBool::create(true));
      }
    }

  private:
    void apply(float progress)
    {
      // Fast first, slowing down to rest
      auto eased = 1 - std::pow(1 - progress, 3.f);
      auto shown = m_in ? eased : 1 - eased;
      auto offset = m_in ? -m_slide * (1 - eased) : m_slide * eased;
      m_pTarget->setPosition({m_home.x + offset, m_home.y});
      for (auto const& [node, opacity] : m_fade.tops)
      {
        if (auto rgba = typeinfo_cast<CCRGBAProtocol*>(node.data()))
          rgba->setOpacity(static_cast<GLubyte>(std::lround(opacity * shown)));
      }
    }

    bool m_in = false;
    bool m_settled = false;
    float m_delay = 0;
    float m_time = 0;
    float m_slide = 0;
    CCPoint m_home;
    Fade m_fade;
  };

  // A wave already moving it ends where it was going first
  void settle(CCNode* node)
  {
    if (auto running = static_cast<WaveAction*>(node->getActionByTag(WAVE_ACTION)))
    {
      running->settle();
      node->stopAction(running);
    }
  }

  float play(ms::wave::Steps const& steps, float delay, bool in)
  {
    auto const& theme = Theme::get();
    auto count = static_cast<int>(steps.size());
    if (count == 0)
      return 0;
    auto stagger = count > 1 ? std::min(theme.pageStagger, theme.pageStaggerTotal / (count - 1)) : 0.f;
    auto time = std::max(theme.pageStepDuration, 0.01f);
    for (int i = 0; i < count; i++)
    {
      auto const& step = steps[i];
      for (auto node : step.nodes)
      {
        if (!node)
          continue;
        settle(node);
        if (in)
        {
          if (!node->isVisible())
          {
            // Hidden by something else than a wave: it stays so
            if (!node->getUserObject(HIDDEN_KEY))
              continue;
            node->setVisible(true);
          }
          node->setUserObject(HIDDEN_KEY, nullptr);
        }
        else if (!node->isVisible())
          continue;
        auto action = WaveAction::create(in, delay + stagger * i, time, step.slide ? theme.pageSlide : 0.f);
        action->setTag(WAVE_ACTION);
        node->runAction(action);
      }
    }
    return delay + stagger * (count - 1) + time;
  }
} // namespace

namespace ms::wave
{
  bool shows(CCNode* node) { return node && (node->isVisible() || node->getUserObject(HIDDEN_KEY)); }

  bool moving(CCNode* node) { return node && node->getActionByTag(WAVE_ACTION); }

  float out(Steps const& steps, float delay) { return play(steps, delay, false); }

  float in(Steps const& steps, float delay) { return play(steps, delay, true); }

  void hide(Steps const& steps)
  {
    for (auto const& step : steps)
    {
      for (auto node : step.nodes)
      {
        if (!node)
          continue;
        settle(node);
        if (!node->isVisible())
          continue;
        node->setVisible(false);
        node->setUserObject(HIDDEN_KEY, CCBool::create(true));
      }
    }
  }

  void show(Steps const& steps)
  {
    for (auto const& step : steps)
    {
      for (auto node : step.nodes)
      {
        if (!node)
          continue;
        settle(node);
        if (!node->getUserObject(HIDDEN_KEY))
          continue;
        node->setVisible(true);
        node->setUserObject(HIDDEN_KEY, nullptr);
      }
    }
  }
} // namespace ms::wave
