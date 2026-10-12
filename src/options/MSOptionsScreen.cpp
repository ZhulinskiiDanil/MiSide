#include "MSOptionsScreen.hpp"

#include "MSAlerts.hpp"
#include "MSOptionsData.hpp"

#include "../Theme.hpp"
#include "../ui/MSStyle.hpp"

using namespace geode::prelude;

namespace
{
  // The info's text gets this much smaller at a time until it fits over the bottom row, down to the smallest
  constexpr float INFO_SHRINK = 0.9f;
  constexpr float INFO_SMALLEST = 0.18f;
  // Between a paragraph and what's under it
  constexpr float TEXT_GAP = 4;

  int s_restyling = 0;
} // namespace

namespace ms::options
{
  CCNode* wrapped(std::string const& text, float scale, ccColor3B color, float width)
  {
    auto const& theme = Theme::get();
    scale = std::max(scale, 0.05f);
    CCNode* node = nullptr;
    if (ms::resourceExists(theme.font))
      node = CCLabelBMFont::create(text.c_str(), theme.font.c_str(), width / scale, kCCTextAlignmentLeft);
    if (!node)
      node = ms::createLabel(text);
    if (!node)
      return nullptr;
    node->setScale(scale);
    node->setAnchorPoint({0, 1});
    ms::setColor(node, color);
    return node;
  }

  bool restyling() { return s_restyling > 0; }

  void holdRestyling(bool hold) { s_restyling = std::max(0, s_restyling + (hold ? 1 : -1)); }
} // namespace ms::options

MSOptionsScreen::~MSOptionsScreen()
{
  for (auto const& layer : m_driven)
    ms::options::dispose(layer);
}

void MSOptionsScreen::onEnter()
{
  MSScreen::onEnter();
  ms::options::holdRestyling(true);
}

void MSOptionsScreen::onExit()
{
  ms::options::holdRestyling(false);
  MSScreen::onExit();
}

bool MSOptionsScreen::initOptions(std::string const& title)
{
  if (!this->initScreen(title))
    return false;
  auto const& theme = Theme::get();
  m_column = MSPauseColumn::create();
  if (!m_column)
    return false;
  m_column->setRowWidth(theme.optionsRowWidth);
  m_column->setOnInfo([this](std::string const& info) { this->showInfo(info); });
  m_top = theme.screenTop;
  return true;
}

void MSOptionsScreen::addText(std::string const& text)
{
  auto const& theme = Theme::get();
  if (text.empty())
    return;
  auto node = ms::options::wrapped(text, theme.achievementsTextScale, theme.achievementsTextColor,
                                   theme.optionsRowWidth - 4);
  if (!node)
    return;
  node->setPosition({theme.screenLeft + 2, m_top + fromTop()});
  m_mainLayer->addChild(node, 10);
  m_texts.push_back(node);
  m_top -= node->getScaledContentSize().height + TEXT_GAP;
}

void MSOptionsScreen::resetOptions()
{
  for (auto node : m_texts)
    node->removeFromParent();
  m_texts.clear();
  this->showInfo("");
  if (m_column)
    m_column->removeFromParent();
  auto const& theme = Theme::get();
  m_column = MSPauseColumn::create();
  m_column->setRowWidth(theme.optionsRowWidth);
  m_column->setOnInfo([this](std::string const& info) { this->showInfo(info); });
  m_top = theme.screenTop;
}

void MSOptionsScreen::finishOptions()
{
  auto const& theme = Theme::get();
  // At least a few rows under the paragraphs
  auto bottom = std::min(theme.optionsViewBottom, m_top - theme.pauseRowPitch * 3);
  m_column->setViewHeight(std::max(m_top - bottom, theme.pauseRowHeight));
  m_mainLayer->addChild(m_column, 10);
  m_column->finish(theme.screenLeft, m_top + fromTop());
}

void MSOptionsScreen::showInfo(std::string const& text)
{
  if (text == m_infoText)
    return;
  m_infoText = text;
  if (m_info)
  {
    m_info->removeFromParent();
    m_info = nullptr;
  }
  if (text.empty())
    return;
  auto const& theme = Theme::get();
  auto top = theme.optionsInfoTop + fromTop();
  auto room = std::max(top - (theme.screenBackTop + fromTop()) - 2, 4.f);
  for (auto scale = std::max(theme.optionsInfoScale, INFO_SMALLEST);; scale *= INFO_SHRINK)
  {
    auto node = ms::options::wrapped(text, scale, theme.optionsInfoColor, theme.optionsRowWidth - 4);
    if (!node)
      return;
    if (node->getScaledContentSize().height > room && scale * INFO_SHRINK >= INFO_SMALLEST)
      continue;
    node->setPosition({theme.screenLeft + 2, top});
    m_mainLayer->addChild(node, 10);
    m_info = node;
    return;
  }
}

void MSOptionsScreen::press(CCNode* window, std::function<void()> const& action)
{
  if (!action)
    return;
  std::vector<CCNode*> before;
  if (window)
  {
    for (auto child : CCArrayExt<CCNode*>(window->getChildren()))
      before.push_back(child);
  }
  action();
  if (!window)
    return;
  std::vector<Ref<CCNode>> added;
  for (auto child : CCArrayExt<CCNode*>(window->getChildren()))
  {
    if (std::find(before.begin(), before.end(), child) == before.end())
      added.emplace_back(child);
  }
  for (auto const& node : added)
  {
    // Only what a click puts up: popups, dialogs, windows; a window's own pieces stay
    if (!typeinfo_cast<FLAlertLayer*>(node.data()) && !typeinfo_cast<DialogLayer*>(node.data()) &&
        !typeinfo_cast<GJDropDownLayer*>(node.data()))
      continue;
    auto z = node->getZOrder();
    node->removeFromParentAndCleanup(false);
    ms::options::adoptPopup(node, z);
  }
}

bool MSOptionsScreen::onKey(enumKeyCodes key) { return m_column && m_column->onKey(key); }

bool MSOptionsScreen::onBack() { return m_column && m_column->cancelEdit(); }
