#include "MSModTray.hpp"

#include "../Theme.hpp"
#include "MSStyle.hpp"

#include <cmath>

using namespace geode::prelude;

namespace
{
  // A button's scale at rest: while its press animation runs, the scale it returns to
  float restingScale(CCNode* node)
  {
    if (auto button = typeinfo_cast<CCMenuItemSpriteExtra*>(node))
    {
      if (button->isSelected() || button->numberOfRunningActions() > 0)
        return button->m_baseScale;
    }
    return node->getScaleX();
  }

  bool isAnimating(CCNode* node)
  {
    auto button = typeinfo_cast<CCMenuItemSpriteExtra*>(node);
    return button && (button->isSelected() || button->numberOfRunningActions() > 0);
  }

  // How much a node is scaled on the screen by its parents
  float worldScale(CCNode* node)
  {
    auto transform = node->nodeToWorldTransform();
    return std::sqrt(transform.a * transform.a + transform.b * transform.b);
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

std::vector<CCNode*> MSModTray::collect(std::vector<float>& signature)
{
  std::vector<CCNode*> buttons;
  auto layer = this->getParent();
  if (!layer)
    return buttons;
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
    auto transform = menu->nodeToWorldTransform();
    signature.insert(signature.end(), {transform.a, transform.b, transform.tx, transform.ty});
    for (auto child : CCArrayExt<CCNode*>(menu->getChildren()))
    {
      auto size = child->getContentSize();
      if (!child->isVisible() || size.width <= 0 || size.height <= 0)
        continue;
      signature.insert(signature.end(), {child->getPositionX(), child->getPositionY(), restingScale(child),
                                         size.width, size.height});
      buttons.push_back(child);
    }
    signature.push_back(-1);
  }
  return buttons;
}

void MSModTray::update(float)
{
  std::vector<float> signature;
  auto buttons = this->collect(signature);
  if (signature == m_signature)
    return;
  this->arrange(buttons);
  // What the buttons look like once placed, so the next frame finds nothing changed
  m_signature.clear();
  this->collect(m_signature);
}

void MSModTray::place(CCNode* button, float size, CCPoint center)
{
  auto menu = button->getParent();
  auto own = button->getContentSize();
  // A menu scaled to nothing (an animation's first frame): placed once it has a size
  auto menuScale = worldScale(menu);
  if (menuScale <= 0.001f)
    return;
  auto scale = size / (std::max(own.width, own.height) * menuScale);
  // The press animation returns to this scale; a running one isn't cut short
  if (auto extra = typeinfo_cast<CCMenuItemSpriteExtra*>(button))
    extra->m_baseScale = scale;
  if (!isAnimating(button))
    button->setScale(scale);

  auto local = menu->convertToNodeSpace(this->convertToWorldSpace(center));
  auto anchor = button->isIgnoreAnchorPointForPosition() ? CCPointZero : button->getAnchorPoint();
  button->setPosition(local - CCPoint{(0.5f - anchor.x) * own.width * scale, (0.5f - anchor.y) * own.height * scale});
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

void MSModTray::arrange(std::vector<CCNode*> const& buttons)
{
  auto const& theme = Theme::get();
  auto count = buttons.size();
  for (auto i = count; i < m_slots.size(); i++)
    m_slots[i]->setVisible(false);
  m_panel->setVisible(count > 0);
  if (count == 0)
    return;

  auto winSize = CCDirector::get()->getWinSize();
  auto columns = std::clamp<size_t>(static_cast<size_t>(std::max(theme.trayColumns, 1.f)), 1, count);
  auto rows = (count + columns - 1) / columns;

  // Too many rows for the screen: every slot gets smaller
  auto outer = theme.slotSize + theme.slotPadding * 2;
  auto height = rows * outer + (rows - 1) * theme.trayGap;
  auto fit = std::min(1.f, winSize.height * theme.trayMaxHeight / height);
  outer *= fit;
  auto gap = theme.trayGap * fit;
  auto inner = theme.slotSize * fit;
  auto pitch = outer + gap;

  // From the bottom row up, each row against the right edge, in order from left to right
  auto right = winSize.width - theme.trayMargin - theme.trayPadding;
  auto bottom = theme.trayMargin + theme.trayPadding;
  for (size_t i = 0; i < count; i++)
  {
    auto row = i / columns;
    auto inRow = row == rows - 1 ? count - row * columns : columns;
    auto column = i % columns;
    CCPoint center{right - outer / 2 - (inRow - 1 - column) * pitch, bottom + outer / 2 + row * pitch};

    auto node = this->slot(i);
    node->setVisible(true);
    node->setContentSize({outer, outer});
    node->setPosition(center);
    this->place(buttons[i], inner, center);
  }

  auto width = columns * outer + (columns - 1) * gap;
  height = rows * outer + (rows - 1) * gap;
  m_panel->setPosition({right - width - theme.trayPadding, bottom - theme.trayPadding});
  m_panel->setContentSize({width + theme.trayPadding * 2, height + theme.trayPadding * 2});
}
