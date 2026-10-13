#include "MSWave.hpp"

#include "../Theme.hpp"
#include "MSMenuItem.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string_view>
#include <typeinfo>

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

  // Whether a node's class is the mod's own: its vtable in this module's code
  bool isOwn(void const* vtable)
  {
#ifdef GEODE_IS_WINDOWS
    static auto const RANGE = [] {
      HMODULE module = nullptr;
      GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         reinterpret_cast<LPCWSTR>(&isOwn), &module);
      auto base = reinterpret_cast<uintptr_t>(module);
      auto dos = reinterpret_cast<IMAGE_DOS_HEADER const*>(base);
      auto nt = reinterpret_cast<IMAGE_NT_HEADERS const*>(base + dos->e_lfanew);
      return std::pair{base, base + nt->OptionalHeader.SizeOfImage};
    }();
    auto address = reinterpret_cast<uintptr_t>(vtable);
    return RANGE.first && address >= RANGE.first && address < RANGE.second;
#else
    static void* const BASE = [] {
      Dl_info info{};
      return dladdr(reinterpret_cast<void*>(&isOwn), &info) ? info.dli_fbase : nullptr;
    }();
    Dl_info info{};
    return BASE && dladdr(vtable, &info) && info.dli_fbase == BASE;
#endif
  }

  // A node whose opacity can be set: exactly one of cocos' plain classes (not GD's or another mod's subclass of
  // one, whose setOpacity may reach members it never set: GJItemIcon's does)
  bool isPlain(CCNode* node)
  {
    auto const& type = typeid(*node);
    return type == typeid(CCSprite) || type == typeid(CCLabelBMFont) || type == typeid(CCLabelTTF) ||
           type == typeid(CCLayerColor) || type == typeid(CCLayerGradient) || type == typeid(CCNodeRGBA) ||
           type == typeid(CCLayerRGBA) || type == typeid(CCScale9Sprite) || type == typeid(CCSpriteBatchNode);
  }

  // The mod's own classes: not faded themselves (their setOpacity may be their own), only what's under them
  bool isSafe(CCNode* node)
  {
    void const* vtable = nullptr;
    std::memcpy(&vtable, static_cast<void const*>(node), sizeof(vtable));
    return isPlain(node) || isOwn(vtable);
  }

  // How a node's look fades. A menu item through its own multiplier (its look is its own, lit or not); the topmost
  // other nodes with an opacity get theirs set, what's under them following through cocos' cascade, turned on for
  // the wave where it was off. Nothing reaches a node of GD's (neither setOpacity nor the cascade's
  // updateDisplayedOpacity is safe on every one: GJItemIcon's reads members some icons never set): those hide while
  // the wave moves them, and show again in place
  struct Fade
  {
    std::vector<Ref<MSMenuItem>> items;
    std::vector<std::pair<Ref<CCNode>, GLubyte>> tops;
    std::vector<Ref<CCNode>> cascaded;
    std::vector<Ref<CCNode>> hidden;
  };

  // A node that isn't safe to fade, in its tree
  bool holdsUnsafe(CCNode* node)
  {
    if (typeinfo_cast<CCRGBAProtocol*>(node) && !isSafe(node))
      return true;
    for (auto child : CCArrayExt<CCNode*>(node->getChildren()))
    {
      if (holdsUnsafe(child))
        return true;
    }
    return false;
  }

  void collect(CCNode* node, bool followsParent, Fade& fade)
  {
    if (auto item = typeinfo_cast<MSMenuItem*>(node))
    {
      fade.items.emplace_back(item);
      return;
    }
    auto rgba = typeinfo_cast<CCRGBAProtocol*>(node);
    if (rgba && !isSafe(node))
    {
      if (node->isVisible())
      {
        node->setVisible(false);
        fade.hidden.emplace_back(node);
      }
      return;
    }
    auto children = CCArrayExt<CCNode*>(node->getChildren());
    auto drives = false;
    // Faded itself only as a plain cocos node with nothing unsafe under it (the cascade must not reach one); else only
    // what's under it is
    if (rgba && isPlain(node) && !holdsUnsafe(node))
    {
      if (!followsParent)
        fade.tops.emplace_back(node, rgba->getOpacity());
      if (!rgba->isCascadeOpacityEnabled())
      {
        rgba->setCascadeOpacityEnabled(true);
        fade.cascaded.emplace_back(node);
      }
      drives = true;
    }
    for (auto child : children)
      collect(child, drives, fade);
  }

  // Back as it was: the items' multipliers, the tops' opacities, then the cascade off again where it was (what's
  // under those shown with their own opacities again), GD's nodes shown
  void restore(Fade const& fade)
  {
    for (auto const& item : fade.items)
      item->setWaveAlpha(1);
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
    for (auto const& node : fade.hidden)
      node->setVisible(true);
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
      for (auto const& item : m_fade.items)
        item->setWaveAlpha(shown);
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
    auto started = 0, idle = 0;
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
        started++;
        idle += node->isRunning() ? 0 : 1;
      }
    }
    log::info("Pages: wave {}, {} nodes, {} not running", in ? "in" : "out", started, idle);
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
