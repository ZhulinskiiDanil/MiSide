#include "MSTransition.hpp"

#include "../Theme.hpp"

using namespace geode::prelude;

MSTransition* MSTransition::create()
{
  auto transition = new MSTransition();
  if (transition->init())
  {
    transition->autorelease();
    return transition;
  }
  delete transition;
  return nullptr;
}

bool MSTransition::init()
{
  auto const& theme = Theme::get();
  if (!CCLayerColor::initWithColor(Theme::withAlpha(theme.fadeColor, 0)))
    return false;
  this->setVisible(false);
  return true;
}

void MSTransition::play(std::function<void()> action, std::function<void()> done)
{
  auto const& theme = Theme::get();
  this->stopAllActions();
  this->setOpacity(0);
  this->setVisible(true);

  auto after = CallFuncExt::create([this, action = std::move(action), done = std::move(done)] {
    if (action)
      action();
    // The scene is being replaced: the vanilla transition starts from the darkened menu
    if (CCDirector::get()->getNextScene())
      return;
    auto const& theme = Theme::get();
    this->runAction(CCSequence::createWithTwoActions(CCFadeTo::create(theme.fadeBackDuration, 0),
                                                     CallFuncExt::create([this, done] {
                                                       this->setVisible(false);
                                                       if (done)
                                                         done();
                                                     })));
  });
  this->runAction(CCSequence::createWithTwoActions(CCFadeTo::create(theme.fadeDuration, theme.fadeOpacity), after));
}
