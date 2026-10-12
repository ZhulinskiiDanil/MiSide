#pragma once

#include "MitaPlan.hpp"

#include <Geode/Geode.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

class MitaPageLoader;
struct MitaDecodedPage;
namespace mita3d { class Renderer; }

// Interactive GLB character, with the existing streamed atlases as a fallback.
// Owns its render resources; clicks spark only on her visible silhouette and pass through.
class MSMita : public cocos2d::CCNode, public cocos2d::CCTargetedTouchDelegate
{
public:
  // Null when the theme turns her off or her files are missing or broken (logged once)
  static MSMita* create();
  ~MSMita() override;
  // Hover uses basis; confirmation overrides it with smile 14 and a forward gaze.
  void setExitHovered(bool value) { m_exitHovered = value; }
  void setExitConfirmation(bool value) { m_exitConfirmation = value; }
  void setMenuGazeTarget(std::optional<cocos2d::CCPoint> target) { m_menuGazeTarget = target; }
  bool is3D() const { return m_renderer != nullptr; }

  // Never claims a touch: it only sparks
  bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;

  void update(float dt) override;
  void visit() override;
  void onEnter() override;
  void onExit() override;

protected:
  struct Atlas
  {
    std::filesystem::path png;
    int width = 0;
    int height = 0;
    int firstFrame = 0;
    int frameCount = 0;
  };

  struct Frame
  {
    int atlas = 0;
    // In the atlas' pixels, from its top left corner
    cocos2d::CCRect rect;
  };

  // A texture slot, as big as the largest page: made once while she's on screen and filled again with each new
  // page. Held by unique_ptr and never moved or copied: geode::Ref's move assignment doesn't release what it held,
  // so a vector of pages shifting on erase leaked the page that was overwritten
  struct Page
  {
    // The page in it; -1: free
    int atlas = -1;
    geode::Ref<cocos2d::CCTexture2D> texture;
    std::vector<geode::Ref<cocos2d::CCSpriteFrame>> frames;
    size_t bytes = 0;
    // Where the page is drawn: its alpha, one byte per square of MASK_STEP pixels (the square's most opaque)
    std::vector<uint8_t> mask;
    unsigned maskWidth = 0;
    unsigned maskHeight = 0;

    Page() = default;
    Page(Page const&) = delete;
    Page& operator=(Page const&) = delete;
    Page(Page&&) = delete;
    Page& operator=(Page&&) = delete;
  };

  bool init() override;
  bool readManifest();
  void start();
  void stop();
  // Turns her off for good after a broken file
  void fail(std::string const& why);

  Page* resident(int atlas);
  bool upload(MitaDecodedPage& decoded, int wanted);
  // Their slots are free again (the textures are kept for the next pages)
  void freePages(std::vector<int> const& atlases);
  void show(int frame);
  void layout();
  // Logs what the sprite really shows and how big, and warns when it isn't what was meant
  void checkSprite();
  // Her own shader for the pink line around her outline; null when it's off or doesn't compile
  void makeOutline();
  // The line's width and color for the texture shown now (the last atlas page is smaller)
  void updateOutline(cocos2d::CCTexture2D* texture);
  // She's drawn at this point of the screen (in the frame shown now)
  bool isDrawnAt(cocos2d::CCPoint world);
  // The white light where she was clicked: a flash that breaks into a few soft blobs flying apart, gone in
  // `mita-spark-duration`
  void spark(cocos2d::CCPoint world);
  bool start3D();
  void update3D(float dt);
  void release3D();

  std::filesystem::path m_modelPath;
  std::unique_ptr<mita3d::Renderer> m_renderer;
  geode::Ref<cocos2d::CCTexture2D> m_renderTexture;
  bool m_use3D = false;
  bool m_exitHovered = false;
  bool m_exitConfirmation = false;
  float m_expression = 0;
  cocos2d::CCPoint m_gaze;
  cocos2d::CCPoint m_headGaze;
  cocos2d::CCPoint m_lastGazeAim;
  float m_gazeHold = 0;
  std::optional<cocos2d::CCPoint> m_menuGazeTarget;
  float m_renderWait = 0;
  unsigned m_contextGeneration = 0;
  int m_renderHeight = 0;

  std::vector<Atlas> m_atlases;
  // Every slot's size in pixels: the largest page's
  std::pair<unsigned, unsigned> m_slotSize{0, 0};
  std::vector<Frame> m_frames;
  cocos2d::CCSize m_canvas;
  // Her drawn part's left edge in the canvas, in pixels: what's kept off the menu
  float m_alphaLeft = 0;
  geode::Ref<cocos2d::CCGLProgram> m_outline;
  GLint m_outlineTexel = -1;
  GLint m_outlineColor = -1;
  GLint m_outlinePremult = -1;
  unsigned m_outlineGeneration = 0;
  cocos2d::CCTexture2D* m_outlineFor = nullptr;
  cocos2d::CCSize m_reference;
  float m_fps = 0;
  std::unique_ptr<MitaPlan> m_plan;
  std::shared_ptr<MitaPageLoader> m_loader;
  std::vector<std::unique_ptr<Page>> m_pages;
  cocos2d::CCSprite* m_sprite = nullptr;
  int m_shownFrame = -1;
  double m_elapsed = 0;
  // How long a page takes from the request to the GPU, measured: what the next request looks ahead by
  double m_loadSeconds = 0;
  cocos2d::CCSize m_laidOutFor;
  cocos2d::CCPoint m_home;
  float m_unit = 1;
  cocos2d::CCPoint m_parallax;
  bool m_failed = false;
};
