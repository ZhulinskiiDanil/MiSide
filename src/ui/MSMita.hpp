#pragma once

#include "MitaPlan.hpp"

#include <Geode/Geode.hpp>

#include <filesystem>
#include <memory>
#include <vector>

class MitaPageLoader;
struct MitaDecodedPage;

// The animated character on the right of the menu, from the atlases in her manifest. The pages stream: at most
// three are on the GPU (the one for now and the next two: a page holds 83 ms of her), decoded ahead on a worker
// thread and uploaded here on the main thread. Her textures and frames belong to this node alone, never to cocos' shared caches. The frame comes from
// the time that passed, so she plays at the same speed at any frame rate
class MSMita : public cocos2d::CCNode
{
public:
  // Null when the theme turns her off or her files are missing or broken (logged once)
  static MSMita* create();
  ~MSMita() override;

  void update(float dt) override;
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
