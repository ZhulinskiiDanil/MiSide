#include "MSScenes.hpp"

#include <Geode/modify/CCDirector.hpp>

using namespace geode::prelude;

namespace
{
  // The frame it was armed on: good for that frame and the next, then it lapses (a mod that opened a popup instead
  // of changing the scene doesn't leave it armed)
  unsigned s_armedOn = 0;
  bool s_armed = false;
  bool s_skipped = false;

  bool armed()
  {
    return s_armed && CCDirector::get()->getTotalFrames() - s_armedOn <= 1;
  }

  // Waits for its scene to be the running one, then calls back once
  class Waiter : public CCNode
  {
  public:
    static Waiter* create(std::function<void()> then)
    {
      auto waiter = new Waiter();
      waiter->init();
      waiter->m_then = std::move(then);
      waiter->autorelease();
      return waiter;
    }

    void onEnter() override
    {
      CCNode::onEnter();
      this->scheduleUpdate();
    }

    void update(float) override
    {
      CCNode* scene = this;
      while (scene->getParent())
        scene = scene->getParent();
      if (CCDirector::get()->getRunningScene() != scene)
        return;
      this->unscheduleUpdate();
      // Stays, idle (going from inside its own update isn't safe)
      auto then = std::move(m_then);
      m_then = nullptr;
      if (then)
        then();
    }

    std::function<void()> m_then;
  };
} // namespace

namespace ms::scenes
{
  void skipNextTransition()
  {
    s_armed = true;
    s_skipped = false;
    s_armedOn = CCDirector::get()->getTotalFrames();
  }

  bool skipped() { return s_skipped; }

  bool comingAtOnce() { return armed(); }

  void whenShown(CCNode* node, std::function<void()> then)
  {
    if (node)
      node->addChild(Waiter::create(std::move(then)));
  }
} // namespace ms::scenes

// The scene GD replaces with a transition, the transition taken off when it's armed
class $modify(MSScenesDirector, CCDirector)
{
  bool replaceScene(CCScene* scene)
  {
    if (armed())
    {
      s_armed = false;
      if (auto transition = typeinfo_cast<CCTransitionScene*>(scene); transition && transition->m_pInScene)
      {
        s_skipped = true;
        return CCDirector::replaceScene(transition->m_pInScene);
      }
    }
    return CCDirector::replaceScene(scene);
  }
};
