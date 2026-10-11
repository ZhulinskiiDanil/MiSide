#pragma once

#include "MSMenuColumn.hpp"
#include "MSProfileCard.hpp"

#include <Geode/Geode.hpp>

#include <string>
#include <vector>

// Keeps the column clear, every frame: the column fits between the profile card in the bottom left corner and
// close-menu, and any other node in its way, a mod's too, is moved to the right of it. Nodes are moved, never
// hidden or removed
class MSLayoutGuard : public cocos2d::CCNode
{
public:
  // card: may be null. skipped: the nodes arranged elsewhere (the tray's menus), left alone here
  static MSLayoutGuard* create(MSMenuColumn* column, MSProfileCard* card, std::vector<std::string> skipped);

  void update(float dt) override;

protected:
  bool init(MSMenuColumn* column, MSProfileCard* card, std::vector<std::string> skipped);
  void keepOut(cocos2d::CCNode* layer);

  geode::Ref<MSMenuColumn> m_column;
  geode::Ref<MSProfileCard> m_card;
  std::vector<std::string> m_skipped;
};
