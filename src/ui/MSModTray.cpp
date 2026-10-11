#include "MSModTray.hpp"

#include "../Theme.hpp"
#include "MSStyle.hpp"

#include <Geode/ui/SimpleAxisLayout.hpp>

using namespace geode::prelude;

namespace
{
  // A button's bounds at rest: its press animation (a scale on CCMenuItemSpriteExtra) doesn't move the tray
  CCRect restingBox(CCNode* child)
  {
    auto scaleX = child->getScaleX();
    auto scaleY = child->getScaleY();
    if (auto button = typeinfo_cast<CCMenuItemSpriteExtra*>(child))
    {
      if (button->isSelected() || button->numberOfRunningActions() > 0)
        scaleX = scaleY = button->m_baseScale;
    }
    auto size = child->getContentSize();
    auto anchor = child->isIgnoreAnchorPointForPosition() ? CCPointZero : child->getAnchorPoint();
    auto position = child->getPosition();
    return {position.x - size.width * scaleX * anchor.x, position.y - size.height * scaleY * anchor.y,
            size.width * scaleX, size.height * scaleY};
  }

  CCRect unite(CCRect const& a, CCRect const& b)
  {
    auto minX = std::min(a.getMinX(), b.getMinX());
    auto minY = std::min(a.getMinY(), b.getMinY());
    return {minX, minY, std::max(a.getMaxX(), b.getMaxX()) - minX, std::max(a.getMaxY(), b.getMaxY()) - minY};
  }

  bool isColumn(CCNode* menu, CCRect const& box)
  {
    auto layout = menu->getLayout();
    if (auto axis = typeinfo_cast<AxisLayout*>(layout))
      return axis->getAxis() == Axis::Column;
    if (auto axis = typeinfo_cast<SimpleAxisLayout*>(layout))
      return axis->getAxis() == Axis::Column;
    return box.size.height > box.size.width;
  }
} // namespace

MSModTray* MSModTray::create(std::vector<std::string> sources)
{
  auto tray = new MSModTray();
  if (tray->init(std::move(sources)))
  {
    tray->autorelease();
    return tray;
  }
  delete tray;
  return nullptr;
}

bool MSModTray::init(std::vector<std::string> sources)
{
  if (!CCNode::init())
    return false;
  auto const& theme = Theme::get();
  m_sources = std::move(sources);
  m_panel = ms::createPanel(theme.traySprite, theme.trayColor, theme.trayOpacity);
  if (!m_panel)
    return false;
  m_panel->setAnchorPoint({0, 0});
  m_panel->ignoreAnchorPointForPosition(false);
  m_panel->setVisible(false);
  this->addChild(m_panel);
  this->scheduleUpdate();
  return true;
}

std::vector<MSModTray::Piece> MSModTray::collect(std::vector<float>& signature)
{
  std::vector<Piece> pieces;
  auto layer = this->getParent();
  if (!layer)
    return pieces;
  for (auto const& id : m_sources)
  {
    auto menu = layer->getChildByID(id);
    if (!menu)
    {
      if (std::find(m_warned.begin(), m_warned.end(), id) == m_warned.end())
      {
        m_warned.push_back(id);
        log::warn("Node {} not found, its buttons aren't in the tray", id);
      }
      continue;
    }
    // Moved by someone else: arranged again
    signature.push_back(menu->getPositionX());
    signature.push_back(menu->getPositionY());
    signature.push_back(menu->getScale());

    Piece piece{menu};
    auto any = false;
    for (auto child : CCArrayExt<CCNode*>(menu->getChildren()))
    {
      if (!child->isVisible())
        continue;
      auto box = restingBox(child);
      signature.insert(signature.end(), {box.origin.x, box.origin.y, box.size.width, box.size.height});
      piece.box = any ? unite(piece.box, box) : box;
      any = true;
    }
    signature.push_back(any ? 1.f : 0.f);
    if (!any || piece.box.size.width <= 0 || piece.box.size.height <= 0)
      continue;
    piece.column = isColumn(menu, piece.box);
    pieces.push_back(piece);
  }
  return pieces;
}

void MSModTray::update(float)
{
  std::vector<float> signature;
  auto pieces = this->collect(signature);
  if (signature == m_signature)
    return;
  this->arrange(pieces);
  // What the menus look like once arranged, so the next frame finds nothing changed
  m_signature.clear();
  this->collect(m_signature);
}

void MSModTray::place(CCNode* menu, float scale, CCPoint corner, CCPoint target)
{
  auto parent = menu->getParent();
  menu->setScale(scale);
  auto from = parent->convertToNodeSpace(menu->convertToWorldSpace(corner));
  auto to = parent->convertToNodeSpace(this->convertToWorldSpace(target));
  menu->setPosition(menu->getPosition() + (to - from));
}

CCNode* MSModTray::slot(size_t index)
{
  auto const& theme = Theme::get();
  while (m_slots.size() <= index)
  {
    auto slot = ms::createPanel(theme.slotSprite, theme.slotColor, theme.slotOpacity);
    slot->setAnchorPoint({0.5f, 0.5f});
    slot->ignoreAnchorPointForPosition(false);
    this->addChild(slot);
    m_slots.push_back(slot);
  }
  return m_slots[index];
}

void MSModTray::arrange(std::vector<Piece> const& pieces)
{
  auto const& theme = Theme::get();
  auto winSize = CCDirector::get()->getWinSize();
  auto right = winSize.width - theme.trayMargin - theme.trayPadding;
  auto bottom = theme.trayMargin + theme.trayPadding;

  // Rows from the bottom up, then the columns side by side above them, all against the right edge
  auto y = bottom;
  for (auto const& piece : pieces)
  {
    if (piece.column)
      continue;
    auto scale = std::min(theme.trayRowHeight / piece.box.size.height,
                          winSize.width * theme.trayMaxWidth / piece.box.size.width);
    this->place(piece.menu, scale, {piece.box.getMaxX(), piece.box.getMinY()}, {right, y});
    y += piece.box.size.height * scale + theme.trayGap;
  }
  auto x = right;
  for (auto const& piece : pieces)
  {
    if (!piece.column)
      continue;
    auto scale = std::min(theme.trayRowHeight / piece.box.size.width,
                          winSize.height * theme.trayMaxHeight / piece.box.size.height);
    this->place(piece.menu, scale, {piece.box.getMaxX(), piece.box.getMinY()}, {x, y});
    x -= piece.box.size.width * scale + theme.trayGap;
  }

  // A slot under every button, the panel under them all
  size_t used = 0;
  CCRect bounds;
  for (auto const& piece : pieces)
  {
    auto toTray = CCAffineTransformConcat(piece.menu->nodeToWorldTransform(), this->worldToNodeTransform());
    for (auto child : CCArrayExt<CCNode*>(piece.menu->getChildren()))
    {
      if (!child->isVisible())
        continue;
      auto box = CCRectApplyAffineTransform(restingBox(child), toTray);
      auto node = this->slot(used++);
      node->setVisible(true);
      node->setContentSize(box.size + CCSize{theme.slotPadding, theme.slotPadding} * 2);
      node->setPosition({box.getMidX(), box.getMidY()});
      bounds = used == 1 ? box : unite(bounds, box);
    }
  }
  for (auto i = used; i < m_slots.size(); i++)
    m_slots[i]->setVisible(false);

  m_panel->setVisible(used > 0);
  if (used > 0)
  {
    auto padding = theme.trayPadding + theme.slotPadding;
    m_panel->setPosition({bounds.getMinX() - padding, bounds.getMinY() - padding});
    m_panel->setContentSize(bounds.size + CCSize{padding, padding} * 2);
  }
}
