#include "MSLayoutGuard.hpp"

#include "../Theme.hpp"

using namespace geode::prelude;

namespace
{
  // A node this big is a backdrop or an overlay, not something in the column's way: it's left where it is
  constexpr float BACKDROP_SHARE = 0.5f;

  // A move smaller than this isn't worth a frame
  constexpr float SETTLED = 0.5f;

  CCRect unite(CCRect const& a, CCRect const& b)
  {
    if (a.size.width <= 0 && a.size.height <= 0)
      return b;
    if (b.size.width <= 0 && b.size.height <= 0)
      return a;
    auto minX = std::min(a.getMinX(), b.getMinX());
    auto minY = std::min(a.getMinY(), b.getMinY());
    return {minX, minY, std::max(a.getMaxX(), b.getMaxX()) - minX, std::max(a.getMaxY(), b.getMaxY()) - minY};
  }

  bool isEmpty(CCRect const& rect) { return rect.size.width <= 0 || rect.size.height <= 0; }

  // What a node shows, in its parent's space. A menu (or any node holding others) by its visible children:
  // its own size is often the whole screen
  CCRect visibleBounds(CCNode* node)
  {
    if (!node || !node->isVisible())
      return {};
    auto winSize = CCDirector::get()->getWinSize();
    auto size = node->getContentSize();
    auto holder = typeinfo_cast<CCMenu*>(node) || size.width <= 0 || size.height <= 0 ||
                  size.width > winSize.width * BACKDROP_SHARE || size.height > winSize.height * BACKDROP_SHARE;
    if (!holder || node->getChildrenCount() == 0)
      return isEmpty(node->boundingBox()) ? CCRect{} : node->boundingBox();
    CCRect inside;
    for (auto child : CCArrayExt<CCNode*>(node->getChildren()))
    {
      if (child->isVisible() && !isEmpty(child->boundingBox()))
        inside = unite(inside, child->boundingBox());
    }
    if (isEmpty(inside))
      return {};
    return CCRectApplyAffineTransform(inside, node->nodeToParentTransform());
  }
} // namespace

MSLayoutGuard* MSLayoutGuard::create(MSMenuColumn* column, MSProfileCard* card, std::vector<std::string> skipped)
{
  auto guard = new MSLayoutGuard();
  if (guard->init(column, card, std::move(skipped)))
  {
    guard->autorelease();
    return guard;
  }
  delete guard;
  return nullptr;
}

bool MSLayoutGuard::init(MSMenuColumn* column, MSProfileCard* card, std::vector<std::string> skipped)
{
  if (!CCNode::init() || !column)
    return false;
  m_column = column;
  m_card = card;
  m_skipped = std::move(skipped);
  for (auto id : {"player-username", "close-menu"})
    m_skipped.emplace_back(id);
  this->scheduleUpdate();
  return true;
}

void MSLayoutGuard::update(float)
{
  auto layer = this->getParent();
  if (!layer || m_column->getParent() != layer)
    return;
  auto const& theme = Theme::get();
  auto winSize = CCDirector::get()->getWinSize();

  // Above the profile card
  auto bottom = theme.trayMargin;
  if (m_card && m_card->getParent() == layer)
  {
    auto card = m_card->getArea();
    if (!isEmpty(card))
      bottom = card.getMaxY();
  }
  bottom += theme.columnClearance;
  auto top = winSize.height * theme.columnTop;
  // Under close-menu's buttons (the mods' ones: the close button itself is hidden, it's the Quit item)
  auto above = visibleBounds(layer->getChildByID("close-menu"));
  if (!isEmpty(above))
    top = std::min(top, above.getMinY() - theme.columnClearance);
  m_column->fit(top, bottom);

  this->keepOut(layer);
}

void MSLayoutGuard::keepOut(CCNode* layer)
{
  auto const& theme = Theme::get();
  auto winSize = CCDirector::get()->getWinSize();
  auto area = m_column->getArea();
  auto clearance = theme.columnClearance;
  area = {area.getMinX() - clearance, area.getMinY() - clearance, area.size.width + clearance * 2,
          area.size.height + clearance * 2};
  auto prefix = Mod::get()->getID() + "/";

  for (auto child : CCArrayExt<CCNode*>(layer->getChildren()))
  {
    if (child == m_column || child == m_card || child == this || !child->isVisible())
      continue;
    std::string_view id = child->getID();
    if (id.starts_with(prefix) || std::find(m_skipped.begin(), m_skipped.end(), id) != m_skipped.end())
      continue;
    auto bounds = visibleBounds(child);
    if (isEmpty(bounds) || bounds.size.width > winSize.width * BACKDROP_SHARE ||
        bounds.size.height > winSize.height * BACKDROP_SHARE || !bounds.intersectsRect(area))
      continue;
    auto shift = area.getMaxX() - bounds.getMinX();
    if (shift > SETTLED)
    {
      log::debug("{} was over the column, moved {} to the right", id.empty() ? "A node" : id, shift);
      child->setPositionX(child->getPositionX() + shift);
    }
  }
}
