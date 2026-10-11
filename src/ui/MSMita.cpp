#include "MSMita.hpp"

#include "../Theme.hpp"

#include <Geode/utils/file.hpp>

#include <Windows.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <chrono>
#include <cmath>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <set>
#include <thread>

using namespace geode::prelude;
using Microsoft::WRL::ComPtr;

// ! --- Decoding on a worker thread --- !

// A page decoded to straight-alpha RGBA, waiting for the main thread to upload it
struct MitaDecodedPage
{
  int atlas = -1;
  bool ok = false;
  unsigned width = 0;
  unsigned height = 0;
  std::vector<uint8_t> pixels;
  double milliseconds = 0;
};

// One worker thread decoding one page at a time with Windows' own PNG decoder (WIC): no cocos object, cache or
// file utility is touched off the main thread. At most one finished page waits, so the CPU holds at most two
// pages' pixels (one finished, one being decoded). The thread owns this state with a shared_ptr and never sees
// the node: once stopped, it drops what it decoded and ends by itself
class MitaPageLoader
{
public:
  static std::shared_ptr<MitaPageLoader> start(std::vector<std::filesystem::path> pngs)
  {
    auto loader = std::make_shared<MitaPageLoader>();
    loader->m_pngs = std::move(pngs);
    std::thread([loader] { loader->run(); }).detach();
    return loader;
  }

  // The latest request replaces the one not started yet
  void request(int atlas)
  {
    std::lock_guard lock(m_mutex);
    if (m_stopped || atlas < 0 || atlas >= static_cast<int>(m_pngs.size()))
      return;
    m_requested = atlas;
    m_wake.notify_one();
  }

  // Requested, being decoded, or decoded and waiting
  bool isBusyWith(int atlas)
  {
    std::lock_guard lock(m_mutex);
    return m_requested == atlas || m_decoding == atlas || (m_ready && m_ready->atlas == atlas);
  }

  // The page decoded and waiting, without taking it: -1 when none
  int readyAtlas()
  {
    std::lock_guard lock(m_mutex);
    return m_ready ? m_ready->atlas : -1;
  }

  std::optional<MitaDecodedPage> take()
  {
    std::lock_guard lock(m_mutex);
    std::optional<MitaDecodedPage> page;
    page.swap(m_ready);
    return page;
  }

  void stop()
  {
    std::lock_guard lock(m_mutex);
    m_stopped = true;
    m_requested = -1;
    m_ready.reset();
    m_wake.notify_one();
  }

private:
  static MitaDecodedPage decode(IWICImagingFactory* factory, std::filesystem::path const& path, int atlas)
  {
    MitaDecodedPage page;
    page.atlas = atlas;
    auto began = std::chrono::steady_clock::now();
    ComPtr<IWICBitmapDecoder> decoder;
    ComPtr<IWICBitmapFrameDecode> frame;
    ComPtr<IWICBitmapSource> rgba;
    if (!factory ||
        FAILED(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand,
                                                  &decoder)) ||
        FAILED(decoder->GetFrame(0, &frame)) ||
        // Straight alpha, as the file has it
        FAILED(WICConvertBitmapSource(GUID_WICPixelFormat32bppRGBA, frame.Get(), &rgba)) ||
        FAILED(rgba->GetSize(&page.width, &page.height)) || page.width == 0 || page.height == 0)
      return page;
    auto stride = page.width * 4;
    page.pixels.resize(static_cast<size_t>(stride) * page.height);
    if (FAILED(rgba->CopyPixels(nullptr, stride, static_cast<UINT>(page.pixels.size()), page.pixels.data())))
    {
      page.pixels.clear();
      return page;
    }
    page.ok = true;
    page.milliseconds =
      std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - began).count();
    return page;
  }

  void run()
  {
    auto com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    {
      ComPtr<IWICImagingFactory> factory;
      CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
      while (true)
      {
        int atlas = -1;
        {
          std::unique_lock lock(m_mutex);
          m_wake.wait(lock, [this] { return m_stopped || m_requested >= 0; });
          if (m_stopped)
            break;
          atlas = m_requested;
          m_requested = -1;
          m_decoding = atlas;
        }
        auto page = decode(factory.Get(), m_pngs[atlas], atlas);
        std::lock_guard lock(m_mutex);
        m_decoding = -1;
        if (m_stopped)
          break;
        // Only one waits: a newer page replaces one nobody took
        m_ready = std::move(page);
      }
    }
    if (SUCCEEDED(com))
      CoUninitialize();
  }

  std::mutex m_mutex;
  std::condition_variable m_wake;
  std::vector<std::filesystem::path> m_pngs;
  int m_requested = -1;
  int m_decoding = -1;
  std::optional<MitaDecodedPage> m_ready;
  bool m_stopped = false;
};

// ! --- The character --- !

namespace
{
  constexpr char const* MANIFEST = "mita-manifest.json";
  // The measured load time weighs this much against the ones before
  constexpr double LOAD_SMOOTHING = 0.3;
  // A request looks ahead by this much more than a load takes
  constexpr double LOAD_MARGIN = 1.5;

  // Each problem once a session, however many times the menu opens
  void logOnce(std::string const& message)
  {
    static std::set<std::string> s_logged;
    if (s_logged.insert(message).second)
      log::error("Mita: {}", message);
  }

  double mebibytes(size_t bytes) { return bytes / (1024.0 * 1024.0); }
} // namespace

MSMita* MSMita::create()
{
  if (!Theme::get().mitaEnabled)
    return nullptr;
  auto mita = new MSMita();
  if (mita->init())
  {
    mita->autorelease();
    return mita;
  }
  delete mita;
  return nullptr;
}

MSMita::~MSMita()
{
  if (m_loader)
    m_loader->stop();
}

bool MSMita::readManifest()
{
  auto dir = Mod::get()->getResourcesDir();
  auto json = file::readJson(dir / MANIFEST);
  if (!json)
  {
    logOnce(fmt::format("can't read {}: {}", MANIFEST, json.unwrapErr()));
    return false;
  }
  auto const& manifest = json.unwrap();
  auto count = manifest["frameCount"].asInt().unwrapOr(0);
  m_fps = static_cast<float>(manifest["fps"].asDouble().unwrapOr(0));
  m_canvas = CCSize(static_cast<float>(manifest["canvas"]["width"].asDouble().unwrapOr(0)),
                    static_cast<float>(manifest["canvas"]["height"].asDouble().unwrapOr(0)));
  auto const& reference = manifest["placement"]["referenceScreen"];
  m_reference = CCSize(static_cast<float>(reference[0].asDouble().unwrapOr(569)),
                       static_cast<float>(reference[1].asDouble().unwrapOr(320)));
  if (count <= 0 || m_canvas.width <= 0 || m_canvas.height <= 0 || m_reference.height <= 0 ||
      !manifest["atlases"].isArray() || !manifest["frames"].isArray())
  {
    logOnce(fmt::format("{} is broken: no frames, canvas or atlases", MANIFEST));
    return false;
  }

  for (auto const& entry : manifest["atlases"])
  {
    Atlas atlas;
    // The atlases are packaged next to the manifest
    auto png = entry["png"].asString().unwrapOr("");
    atlas.png = dir / std::filesystem::path(png).filename();
    atlas.width = static_cast<int>(entry["size"][0].asInt().unwrapOr(0));
    atlas.height = static_cast<int>(entry["size"][1].asInt().unwrapOr(0));
    atlas.firstFrame = static_cast<int>(entry["firstFrame"].asInt().unwrapOr(-1));
    atlas.frameCount = static_cast<int>(entry["frameCount"].asInt().unwrapOr(0));
    std::error_code error;
    if (png.empty() || atlas.width <= 0 || atlas.height <= 0 || atlas.firstFrame < 0 || atlas.frameCount <= 0 ||
        !std::filesystem::exists(atlas.png, error))
    {
      logOnce(fmt::format("atlas {} is missing or described wrong in {}", png, MANIFEST));
      return false;
    }
    m_atlases.push_back(atlas);
  }

  for (auto const& entry : manifest["frames"])
  {
    Frame frame;
    frame.atlas = static_cast<int>(entry["atlas"].asInt().unwrapOr(-1));
    auto const& rect = entry["rect"];
    frame.rect = {static_cast<float>(rect[0].asDouble().unwrapOr(-1)), static_cast<float>(rect[1].asDouble().unwrapOr(-1)),
                  static_cast<float>(rect[2].asDouble().unwrapOr(0)), static_cast<float>(rect[3].asDouble().unwrapOr(0))};
    m_frames.push_back(frame);
  }
  if (static_cast<int>(m_frames.size()) != count)
  {
    logOnce(fmt::format("{} lists {} frames, frameCount says {}", MANIFEST, m_frames.size(), count));
    return false;
  }
  // Every frame inside its atlas, and every atlas' frames where the manifest says
  for (size_t i = 0; i < m_frames.size(); i++)
  {
    auto const& frame = m_frames[i];
    if (frame.atlas < 0 || frame.atlas >= static_cast<int>(m_atlases.size()))
    {
      logOnce(fmt::format("frame {} is in an atlas that isn't listed", i));
      return false;
    }
    auto const& atlas = m_atlases[frame.atlas];
    auto index = static_cast<int>(i);
    if (index < atlas.firstFrame || index >= atlas.firstFrame + atlas.frameCount || frame.rect.origin.x < 0 ||
        frame.rect.origin.y < 0 || frame.rect.size.width <= 0 || frame.rect.size.height <= 0 ||
        frame.rect.getMaxX() > atlas.width || frame.rect.getMaxY() > atlas.height)
    {
      logOnce(fmt::format("frame {} doesn't fit atlas {}", i, frame.atlas));
      return false;
    }
  }
  return true;
}

bool MSMita::init()
{
  if (!CCNode::init() || !this->readManifest())
    return false;
  auto const& theme = Theme::get();
  if (theme.mitaFps > 0)
    m_fps = theme.mitaFps;
  if (m_fps <= 0)
  {
    logOnce("no frame rate in the theme or the manifest");
    return false;
  }
  m_plan = std::make_unique<MitaPlan>(static_cast<int>(m_atlases.size()));
  this->setContentSize(CCDirector::get()->getWinSize());
  return true;
}

void MSMita::onEnter()
{
  CCNode::onEnter();
  this->start();
}

void MSMita::onExit()
{
  // Nothing of hers outlives the menu being on screen: the pages are loaded again if it comes back
  this->stop();
  CCNode::onExit();
}

void MSMita::start()
{
  if (m_failed || m_loader)
    return;
  std::vector<std::filesystem::path> pngs;
  for (auto const& atlas : m_atlases)
    pngs.push_back(atlas.png);
  m_loader = MitaPageLoader::start(std::move(pngs));
  this->scheduleUpdate();
  // The page for where her loop is now, right away
  auto index = MitaPlan::frameAt(m_elapsed, m_fps, static_cast<int>(m_frames.size()));
  m_loader->request(m_frames[index].atlas);
}

void MSMita::stop()
{
  this->unscheduleUpdate();
  if (m_loader)
  {
    m_loader->stop();
    m_loader.reset();
  }
  // Ours: the sprite holds the shown page's texture
  if (m_sprite)
  {
    m_sprite->removeFromParentAndCleanup(true);
    m_sprite = nullptr;
  }
  m_shownFrame = -1;
  if (m_plan)
    this->freePages(m_plan->clear());
  if (!m_pages.empty())
    log::warn("Mita: {} pages left after freeing them all", m_pages.size());
  m_pages.clear();
}

void MSMita::fail(std::string const& why)
{
  logOnce(why);
  m_failed = true;
  this->stop();
}

MSMita::Page* MSMita::resident(int atlas)
{
  for (auto& page : m_pages)
  {
    if (page.atlas == atlas)
      return &page;
  }
  return nullptr;
}

void MSMita::freePages(std::vector<int> const& atlases)
{
  for (auto atlas : atlases)
  {
    auto before = m_pages.size();
    std::erase_if(m_pages, [&](Page const& page) { return page.atlas == atlas; });
    if (m_pages.size() != before)
      log::debug("Mita: atlas {} freed, {} pages on the GPU", atlas, m_pages.size());
  }
}

bool MSMita::upload(MitaDecodedPage& decoded, int wanted)
{
  auto const& atlas = m_atlases[decoded.atlas];
  if (static_cast<int>(decoded.width) != atlas.width || static_cast<int>(decoded.height) != atlas.height)
  {
    this->fail(fmt::format("{} is {}×{}, the manifest says {}×{}", atlas.png.filename(), decoded.width,
                           decoded.height, atlas.width, atlas.height));
    return false;
  }
  // Never a third page on the GPU, even for a moment: room is made first, never from the shown page
  this->freePages(m_plan->makeRoom(wanted));
  if (m_pages.size() >= 2)
  {
    log::warn("Mita: no room for atlas {}, it's skipped", decoded.atlas);
    return true;
  }

  auto began = std::chrono::steady_clock::now();
  auto texture = new CCTexture2D();
  // RGBA8888, straight alpha (a texture made from raw data isn't premultiplied)
  if (!texture->initWithData(decoded.pixels.data(), kCCTexture2DPixelFormat_RGBA8888, decoded.width,
                             decoded.height, CCSize(decoded.width, decoded.height)))
  {
    texture->release();
    this->fail(fmt::format("can't upload {}", atlas.png.filename()));
    return false;
  }
  ccTexParams params{GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE};
  texture->setTexParameters(&params);

  Page page;
  page.atlas = decoded.atlas;
  page.texture = texture;
  texture->release();
  page.bytes = decoded.pixels.size();
  // The manifest's rects and canvas in pixels, as they are: this form of createWithTexture takes pixels and makes
  // the points itself (divided by the content scale factor once, like frames from a plist)
  for (int i = 0; i < atlas.frameCount; i++)
  {
    auto const& rect = m_frames[atlas.firstFrame + i].rect;
    page.frames.emplace_back(CCSpriteFrame::createWithTexture(texture, rect, false, CCPointZero, m_canvas));
  }
  auto uploadMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - began).count();
  auto loadSeconds = (decoded.milliseconds + uploadMs) / 1000;
  m_loadSeconds = m_loadSeconds <= 0 ? loadSeconds : m_loadSeconds + (loadSeconds - m_loadSeconds) * LOAD_SMOOTHING;
  m_pages.push_back(std::move(page));
  m_plan->uploaded(decoded.atlas);

  size_t resident = 0;
  for (auto const& each : m_pages)
    resident += each.bytes;
  log::debug("Mita: atlas {} decoded in {:.1f} ms, uploaded in {:.1f} ms; {} pages on the GPU, {:.2f} MiB",
             decoded.atlas, decoded.milliseconds, uploadMs, m_pages.size(), mebibytes(resident));
  // Its CPU copy goes now
  decoded.pixels = {};
  return true;
}

void MSMita::layout()
{
  auto const& theme = Theme::get();
  auto winSize = CCDirector::get()->getWinSize();
  m_laidOutFor = winSize;
  // The reference screen scaled to the window's height, the same way both ways
  m_unit = winSize.height / m_reference.height;
  auto height = m_canvas.height * theme.mitaScale * m_unit;
  // From the frame's own size in points, whatever the texture quality: never scaled twice. The parents' scale
  // counts too, so the size on screen is the one asked for
  auto size = m_sprite->getContentSize();
  auto parents = this->nodeToWorldTransform();
  auto parentScale = std::sqrt(parents.a * parents.a + parents.b * parents.b);
  if (parentScale <= 0)
    parentScale = 1;
  if (size.height > 0)
    m_sprite->setScale(height / (size.height * parentScale));
  auto width = size.width * m_sprite->getScale() * parentScale;

  // Held to the right edge, as on the reference screen; never over the menu's buttons
  auto x = winSize.width - (m_reference.width - theme.mitaX) * m_unit;
  auto keepClear = theme.columnLeft + theme.rowWidth + theme.columnClearance;
  x = std::max(x, keepClear + width / 2);
  m_home = CCPoint(x, theme.mitaY * m_unit);
}

void MSMita::show(int index)
{
  auto const& frame = m_frames[index];
  auto page = this->resident(frame.atlas);
  if (!page || index == m_shownFrame)
    return;
  auto const& atlas = m_atlases[frame.atlas];
  auto spriteFrame = page->frames[index - atlas.firstFrame].data();
  if (!m_sprite)
  {
    m_sprite = CCSprite::createWithSpriteFrame(spriteFrame);
    if (!m_sprite)
    {
      this->fail("can't make her sprite");
      return;
    }
    m_sprite->setAnchorPoint({0.5f, 0});
    this->addChild(m_sprite);
    this->layout();
    m_sprite->setPosition(m_home);
    this->checkSprite();
    auto const& theme = Theme::get();
    if (theme.introDuration > 0)
    {
      m_sprite->setOpacity(0);
      m_sprite->runAction(CCFadeTo::create(theme.introDuration, 255));
    }
  }
  else
    m_sprite->setDisplayFrame(spriteFrame);
  // Set again after the texture changes: premultiplied or not, the color is never multiplied twice
  auto premultiplied = page->texture->hasPremultipliedAlpha();
  m_sprite->setBlendFunc({static_cast<GLenum>(premultiplied ? GL_ONE : GL_SRC_ALPHA), GL_ONE_MINUS_SRC_ALPHA});
  m_shownFrame = index;
  // The page before is done with
  this->freePages(m_plan->showed(frame.atlas));
}

void MSMita::checkSprite()
{
  // What cocos actually made of it, once each time the menu opens: the frame's rect in the texture's pixels, the
  // size in points, and the size and place on the screen
  auto const& theme = Theme::get();
  auto scale = CC_CONTENT_SCALE_FACTOR();
  auto rect = m_sprite->getTextureRect();
  CCRect pixels{rect.origin.x * scale, rect.origin.y * scale, rect.size.width * scale, rect.size.height * scale};
  auto content = m_sprite->getContentSize();
  auto box = CCRectApplyAffineTransform(CCRect{CCPointZero, content}, m_sprite->nodeToWorldTransform());
  auto expectedHeight = m_canvas.height * theme.mitaScale * m_unit;
  auto expectedWidth = m_canvas.width * theme.mitaScale * m_unit;
  log::debug("Mita: content scale {}, texture rect {}x{} px at ({}, {}), content {}x{} pt, scale {:.4f}, on screen "
             "{:.3f}x{:.3f} at ({:.2f}, {:.2f})",
             scale, pixels.size.width, pixels.size.height, pixels.origin.x, pixels.origin.y, content.width,
             content.height, m_sprite->getScale(), box.size.width, box.size.height, box.getMidX(), box.getMinY());
  if (std::abs(pixels.size.width - m_canvas.width) > 0.5f || std::abs(pixels.size.height - m_canvas.height) > 0.5f ||
      std::abs(box.size.height - expectedHeight) > 0.5f || std::abs(box.size.width - expectedWidth) > 0.5f)
    log::warn("Mita: she's drawn {:.3f}x{:.3f} from a {}x{} px rect, {:.3f}x{:.3f} from {}x{} was meant", box.size.width,
              box.size.height, pixels.size.width, pixels.size.height, expectedWidth, expectedHeight, m_canvas.width,
              m_canvas.height);
}

void MSMita::update(float dt)
{
  if (m_failed || !m_loader)
    return;
  auto const& theme = Theme::get();
  auto count = static_cast<int>(m_frames.size());
  // From the time that passed: the same speed at any frame rate, and a late frame jumps to where it should be
  m_elapsed = MitaPlan::advance(m_elapsed, dt, m_fps, count);
  auto index = MitaPlan::frameAt(m_elapsed, m_fps, count);
  // A still frame, to check one by one
  if (theme.mitaFreezeFrame >= 0)
    index = std::clamp(static_cast<int>(theme.mitaFreezeFrame), 0, count - 1);
  auto atlas = m_frames[index].atlas;

  // The page for now first, if it's there: switching frees the page before, making room for the next
  if (this->resident(atlas))
    this->show(index);

  // A finished page: on the GPU if it's for now or next, left waiting decoded if it's for soon, else dropped
  if (auto ready = m_loader->readyAtlas(); ready >= 0)
  {
    auto use = m_plan->decide(ready, atlas);
    if (use != MitaPlan::Use::Wait)
    {
      auto decoded = m_loader->take();
      if (decoded && !decoded->ok)
      {
        this->fail(fmt::format("can't decode {}", m_atlases[decoded->atlas].png.filename()));
        return;
      }
      if (decoded && use == MitaPlan::Use::Upload && !this->upload(*decoded, atlas))
        return;
    }
  }

  // Just uploaded, or not there yet: then the last frame stays until it is, and the loop catches up with the time
  if (this->resident(atlas))
    this->show(index);
  // Where playback will be once the next load ends
  auto later = MitaPlan::advance(m_elapsed, m_loadSeconds * LOAD_MARGIN, m_fps, count);
  auto predicted = m_frames[MitaPlan::frameAt(later, m_fps, count)].atlas;
  if (auto request = m_plan->toRequest(atlas, predicted); request >= 0 && !m_loader->isBusyWith(request))
    m_loader->request(request);

  if (!m_sprite)
    return;
  if (CCDirector::get()->getWinSize() != m_laidOutFor)
    this->layout();
  CCPoint target;
  if (theme.mitaParallaxEnabled)
  {
    // Never takes the mouse: only reads where it is. Down only, so the cut at the bottom never shows a gap
    auto winSize = CCDirector::get()->getWinSize();
    auto mouse = getMousePos();
    auto aimX = std::clamp((mouse.x - winSize.width / 2) / (winSize.width / 2), -1.f, 1.f);
    auto aimY = std::clamp((mouse.y - winSize.height / 2) / (winSize.height / 2), -1.f, 1.f);
    target = CCPoint(-aimX * theme.mitaParallaxMaxX, -(aimY + 1) / 2 * theme.mitaParallaxMaxY);
  }
  auto follow = theme.parallaxSmoothing > 0 ? 1.f - std::exp(-dt / theme.parallaxSmoothing) : 1.f;
  m_parallax = m_parallax + (target - m_parallax) * follow;
  m_sprite->setPosition(m_home + m_parallax * m_unit);
}
