#include "MSScrollView.hpp"

#include "../Theme.hpp"
#include "MSStyle.hpp"

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace
{
  // The 9-slice's corners are 5.3 pt: thin pieces are made bigger and scaled down
  constexpr float SMALL_PANEL = 4;
  // A press that moves this far scrolls instead of tapping
  constexpr float DRAG_START = 4;
  constexpr float MIN_THUMB = 8;

  CCNode* bar(ccColor3B color, float width, float height)
  {
    auto node = ms::createPanel(Theme::get().rowSprite, color, 255);
    node->setAnchorPoint({0, 1});
    node->ignoreAnchorPointForPosition(false);
    node->setContentSize(CCSize{width, height} * SMALL_PANEL);
    node->setScale(1 / SMALL_PANEL);
    return node;
  }

  // Only what's inside this node's own rectangle draws (the scissor box)
  class Clip : public CCNode
  {
  public:
    static Clip* create()
    {
      auto clip = new Clip();
      clip->init();
      clip->autorelease();
      return clip;
    }

    void visit() override
    {
      if (!m_bVisible)
        return;
      auto from = this->convertToWorldSpace({0, 0});
      auto to = this->convertToWorldSpace(CCPoint{m_obContentSize.width, m_obContentSize.height});
      auto was = glIsEnabled(GL_SCISSOR_TEST);
      GLint box[4] = {0, 0, 0, 0};
      if (was)
        glGetIntegerv(GL_SCISSOR_BOX, box);
      glEnable(GL_SCISSOR_TEST);
      CCEGLView::sharedOpenGLView()->setScissorInPoints(from.x, from.y, to.x - from.x, to.y - from.y);
      CCNode::visit();
      if (was)
        glScissor(box[0], box[1], box[2], box[3]);
      else
        glDisable(GL_SCISSOR_TEST);
    }
  };
} // namespace

MSScrollView* MSScrollView::create(CCRect view, float barLeft, float barWidth, ccColor3B track, ccColor3B thumb)
{
  auto scroll = new MSScrollView();
  if (scroll->init(view, barLeft, barWidth, track, thumb))
  {
    scroll->autorelease();
    return scroll;
  }
  delete scroll;
  return nullptr;
}

bool MSScrollView::init(CCRect view, float barLeft, float barWidth, ccColor3B track, ccColor3B thumb)
{
  if (!CCMenu::init())
    return false;
  m_view = view;
  m_barWidth = barWidth;
  this->setPosition({0, 0});
  this->setContentSize({0, 0});

  // Cut at the top and bottom only: the whole width, for the rows sliding past the bar as a page goes
  auto clip = Clip::create();
  clip->setPosition({0, view.origin.y});
  clip->setContentSize({std::max({barLeft - 1, view.getMaxX(), CCDirector::get()->getWinSize().width}),
                        view.size.height});
  this->addChild(clip);
  m_clip = clip;
  m_content = CCNode::create();
  clip->addChild(m_content);

  m_track = bar(track, barWidth, view.size.height);
  m_track->setPosition({barLeft, view.getMaxY()});
  this->addChild(m_track, -1);
  m_thumb = bar(thumb, barWidth, MIN_THUMB);
  m_thumb->setPosition({barLeft, view.getMaxY()});
  this->addChild(m_thumb, -1);

  this->setMouseEnabled(true);
  this->place();
  this->scheduleUpdate();
  return true;
}

CCNode* MSScrollView::content() const { return m_content; }

ms::wave::Steps MSScrollView::pageSteps()
{
  ms::wave::Steps steps;
  ms::wave::Step bar;
  bar.slide = false;
  for (auto node : {m_track, m_thumb})
  {
    if (ms::wave::shows(node))
      bar.nodes.push_back(node);
  }
  if (!bar.nodes.empty())
    steps.push_back(std::move(bar));
  // In this node's space: the clip's and the content's offsets added
  auto offset = m_clip->getPosition() + m_content->getPosition();
  for (auto child : CCArrayExt<CCNode*>(m_content->getChildren()))
  {
    if (!ms::wave::shows(child))
      continue;
    auto box = child->boundingBox();
    box.origin += offset;
    if (box.getMaxY() > m_view.getMinY() && box.getMinY() < m_view.getMaxY())
      steps.push_back({{child}});
  }
  return steps;
}

float MSScrollView::scroll() const { return m_scroll; }

void MSScrollView::setStep(float step) { m_step = std::max(step, 1.f); }

void MSScrollView::setOnTap(std::function<void(CCPoint)> tap) { m_onTap = std::move(tap); }

void MSScrollView::setActive(bool active) { m_active = active; }

void MSScrollView::setContentHeight(float height)
{
  m_height = std::max(height, 0.f);
  m_target = std::clamp(m_target, 0.f, this->maxScroll());
  m_scroll = std::clamp(m_scroll, 0.f, this->maxScroll());
  this->place();
}

float MSScrollView::maxScroll() const { return std::max(0.f, m_height - m_view.size.height); }

bool MSScrollView::canUse() { return m_active && this->isVisible() && !ms::coveredBy(this); }

void MSScrollView::reveal(float top, float bottom)
{
  if (top < m_target)
    m_target = top;
  else if (bottom > m_target + m_view.size.height)
    m_target = std::min(top, bottom - m_view.size.height);
  m_target = std::clamp(m_target, 0.f, this->maxScroll());
}

void MSScrollView::scrollBy(float amount) { m_target = std::clamp(m_target + amount, 0.f, this->maxScroll()); }

void MSScrollView::place()
{
  // The content's top left corner at the view's, moved up by the scroll
  m_content->setPosition({m_view.origin.x, m_view.size.height + m_scroll});
  auto view = m_view.size.height;
  auto share = m_height > 0 ? std::min(1.f, view / m_height) : 1.f;
  auto thumb = std::max(view * share, MIN_THUMB);
  m_thumb->setContentSize(CCSize{m_barWidth, thumb} * SMALL_PANEL);
  auto most = this->maxScroll();
  auto progress = most > 0 ? std::clamp(m_scroll / most, 0.f, 1.f) : 0.f;
  m_thumb->setPositionY(m_view.getMaxY() - std::max(view - thumb, 0.f) * progress);
  m_track->setVisible(share < 1);
  m_thumb->setVisible(share < 1);
}

bool MSScrollView::scrollWheel(float y, float x)
{
  this->scrollWheelImpl(y, x);
  return true;
}

void MSScrollView::scrollWheelImpl(float y, float)
{
  if (!this->canUse() || y == 0)
    return;
  this->scrollBy((y > 0 ? 1 : -1) * m_step);
}

void MSScrollView::update(float dt)
{
  if (m_scroll == m_target)
    return;
  auto smoothing = Theme::get().achievementsScrollSmoothing;
  auto follow = smoothing > 0 ? 1.f - std::exp(-std::min(dt, 0.1f) / smoothing) : 1.f;
  m_scroll += (m_target - m_scroll) * follow;
  if (std::abs(m_target - m_scroll) < 0.05f)
    m_scroll = m_target;
  this->place();
}

CCRect MSScrollView::trackRect() const
{
  auto box = m_track->boundingBox();
  // Easier to catch than its width
  return {box.origin.x - 4, box.origin.y, box.size.width + 8, box.size.height};
}

bool MSScrollView::ccTouchBegan(CCTouch* touch, CCEvent* event)
{
  if (!this->canUse())
    return false;
  auto local = this->convertTouchToNodeSpace(touch);
  if (m_track->isVisible() && this->trackRect().containsPoint(local))
  {
    m_dragging = true;
    this->ccTouchMoved(touch, event);
    return true;
  }
  if (!m_view.containsPoint(local))
    return false;
  m_touching = true;
  m_moved = false;
  m_touchStart = local;
  m_scrollStart = m_scroll;
  return true;
}

void MSScrollView::ccTouchMoved(CCTouch* touch, CCEvent*)
{
  auto local = this->convertTouchToNodeSpace(touch);
  if (m_dragging)
  {
    // The thumb's middle follows the touch
    auto box = m_track->boundingBox();
    auto thumb = m_thumb->getScaledContentSize().height;
    auto travel = std::max(box.size.height - thumb, 1.f);
    auto progress = std::clamp((box.getMaxY() - thumb / 2 - local.y) / travel, 0.f, 1.f);
    m_target = m_scroll = progress * this->maxScroll();
    this->place();
    return;
  }
  if (!m_touching)
    return;
  auto dy = local.y - m_touchStart.y;
  if (!m_moved && std::abs(dy) < DRAG_START)
    return;
  m_moved = true;
  // The content follows the finger: up scrolls on
  m_target = m_scroll = std::clamp(m_scrollStart + dy, 0.f, this->maxScroll());
  this->place();
}

void MSScrollView::ccTouchEnded(CCTouch* touch, CCEvent*)
{
  if (m_dragging)
  {
    m_dragging = false;
    return;
  }
  if (!m_touching)
    return;
  m_touching = false;
  if (m_moved || !m_onTap)
    return;
  m_onTap(m_content->convertToNodeSpace(touch->getLocation()));
}

void MSScrollView::ccTouchCancelled(CCTouch*, CCEvent*)
{
  m_dragging = false;
  m_touching = false;
  m_moved = false;
}
