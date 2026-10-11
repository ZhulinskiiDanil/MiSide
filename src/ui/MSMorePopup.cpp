#include "MSMorePopup.hpp"

#include "../Theme.hpp"
#include "MSStyle.hpp"

#include <cmath>

using namespace geode::prelude;

namespace
{
  // A copy of what the button shows: its sprite's current frame (a texture pack's too), or its node ID
  CCNode* lookOf(CCMenuItem* target)
  {
    if (auto item = typeinfo_cast<CCMenuItemSprite*>(target))
    {
      if (auto sprite = typeinfo_cast<CCSprite*>(item->getNormalImage()))
      {
        if (auto copy = CCSprite::createWithSpriteFrame(sprite->displayFrame()))
          return copy;
      }
    }
    std::string id = target->getID();
    return ms::createText(id.empty() ? "?" : id, Theme::get().nameScale, Theme::get().itemColor).node;
  }
} // namespace

MSMorePopup* MSMorePopup::create(std::vector<CCMenuItem*> targets, std::string const& title)
{
  auto popup = new MSMorePopup();
  if (popup->init(targets, title))
  {
    popup->autorelease();
    return popup;
  }
  delete popup;
  return nullptr;
}

bool MSMorePopup::init(std::vector<CCMenuItem*> const& targets, std::string const& title)
{
  auto const& theme = Theme::get();
  auto count = std::max<size_t>(targets.size(), 1);
  auto columns = std::clamp<size_t>(static_cast<size_t>(std::max(theme.popupColumns, 1.f)), 1, count);
  auto rows = (count + columns - 1) / columns;
  auto slot = theme.popupSlotSize;
  auto gap = theme.popupGap;
  auto padding = theme.popupPadding;
  // Room for the title over the buttons
  auto titleRoom = padding * 2;
  CCSize size{padding * 2 + columns * slot + (columns - 1) * gap,
              padding * 2 + rows * slot + (rows - 1) * gap + titleRoom};

  auto background = ms::resourceExists(theme.traySprite) ? theme.traySprite : std::string("GJ_square01.png");
  if (!Popup::init(size, background.c_str()))
    return false;
  m_bgSprite->setColor(theme.popupColor);
  m_bgSprite->setOpacity(theme.popupOpacity);

  if (auto text = ms::createText(title, theme.popupTitleScale, theme.subtitleColor); text.node)
  {
    text.node->setAnchorPoint({0.5f, 0.5f});
    text.node->setPosition({size.width / 2, size.height - padding - text.node->getContentHeight() / 2});
    m_mainLayer->addChild(text.node);
  }

  for (size_t i = 0; i < targets.size(); i++)
  {
    Ref<CCMenuItem> target = targets[i];
    auto look = lookOf(target);
    if (!look)
      continue;
    auto own = look->getContentSize();
    if (own.width > 0 && own.height > 0)
      look->setScale(slot / std::max(own.width, own.height));
    auto button = CCMenuItemExt::createSpriteExtra(look, [target](auto) { target->activate(); });

    auto row = i / columns;
    auto column = i % columns;
    // The last row centered when it isn't full
    auto inRow = row == rows - 1 ? targets.size() - row * columns : columns;
    auto rowWidth = inRow * slot + (inRow - 1) * gap;
    CCPoint center{(size.width - rowWidth) / 2 + slot / 2 + column * (slot + gap),
                   size.height - titleRoom - padding - slot / 2 - row * (slot + gap)};
    m_buttonMenu->addChildAtPosition(button, Anchor::BottomLeft, center);
  }
  return true;
}
