#pragma once

#include "MSMenuColumn.hpp"

#include <Geode/Geode.hpp>

#include <string>
#include <vector>

// Keeps the menu clear, every frame: any node in its way, a mod's too, is moved to the right of it. Nodes are
// moved, never hidden or removed
class MSLayoutGuard : public cocos2d::CCNode
{
public:
  // skipped: the nodes arranged elsewhere (the tray's menus), left alone here
  static MSLayoutGuard* create(MSMenuColumn* column, std::vector<std::string> skipped);

  void update(float dt) override;

protected:
  bool init(MSMenuColumn* column, std::vector<std::string> skipped);
  void keepOut(cocos2d::CCNode* layer);

  geode::Ref<MSMenuColumn> m_column;
  std::vector<std::string> m_skipped;
};
