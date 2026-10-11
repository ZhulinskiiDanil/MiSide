#pragma once

#include <Geode/Geode.hpp>

// What's behind the pause menu: the level as it stopped, captured once and blurred (downsampled, then a two-pass
// Gaussian), with a pink-lilac tint and a gradient from the left over it. The blurred picture is kept until the
// window's size changes; nothing is captured or blurred every frame. Only the level is drawn into it, never the
// pause menu. When the blur can't be made here (or it's off), a plain darkening instead
class MSPauseBackdrop : public cocos2d::CCNode
{
public:
  // pause: hidden while the level is drawn, in case it's inside the level's layer
  static MSPauseBackdrop* create(cocos2d::CCNode* pause);
  ~MSPauseBackdrop() override;

  void update(float dt) override;
  bool isBlurred() const;

protected:
  // A framebuffer with its own texture (and a depth-stencil buffer for drawing the level)
  struct Target
  {
    GLuint fbo = 0;
    GLuint depthStencil = 0;
    geode::Ref<cocos2d::CCTexture2D> texture;
    int width = 0;
    int height = 0;
  };

  bool init(cocos2d::CCNode* pause);
  // Captures the level and blurs it into m_result; false (logged) when it can't, and nothing is left made
  bool blur();
  bool makeTarget(Target& target, int width, int height, bool depthStencil);
  void freeTarget(Target& target);
  // One full-screen pass from a texture into a target with this program
  void pass(cocos2d::CCGLProgram* program, cocos2d::CCTexture2D* input, Target const& output);
  void layout();
  void showDim();

  cocos2d::CCNode* m_pause = nullptr;
  Target m_result;
  cocos2d::CCSprite* m_picture = nullptr;
  cocos2d::CCLayerColor* m_dim = nullptr;
  cocos2d::CCLayerColor* m_tint = nullptr;
  cocos2d::CCLayerGradient* m_gradient = nullptr;
  // The window's pixels the picture was made for; a new size waits to settle before it's made again
  cocos2d::CCSize m_frame;
  cocos2d::CCSize m_pendingFrame;
  float m_settle = 0;
};
