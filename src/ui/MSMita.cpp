#include "MSMita.hpp"

#include "../Theme.hpp"
#include "MSStyle.hpp"
#include "../mita3d/Renderer.hpp"

#include <Geode/utils/file.hpp>

#ifdef GEODE_IS_WINDOWS
#include <Windows.h>
#include <wincodec.h>
#include <wrl/client.h>
#else
// Its code is in StbImage.cpp
#include <stb_image.h>
#endif

#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <optional>
#include <set>
#include <thread>

using namespace geode::prelude;
#ifdef GEODE_IS_WINDOWS
using Microsoft::WRL::ComPtr;
#endif

// ! --- Decoding on a worker thread --- !

// A page decoded to straight-alpha RGBA, waiting for the main thread to upload it. Its pixels fill the top left of
// a buffer as big as every texture slot (the last page is half as wide: the rest is transparent)
struct MitaDecodedPage
{
  int atlas = -1;
  bool ok = false;
  // The file's own size
  unsigned width = 0;
  unsigned height = 0;
  std::vector<uint8_t> pixels;
  // Its alpha in squares of MASK_STEP pixels, the square's most opaque: what a click is tested against
  std::vector<uint8_t> mask;
  unsigned maskWidth = 0;
  unsigned maskHeight = 0;
  double milliseconds = 0;
};

namespace
{
  // Renderer owns the GL texture; this adapter only lets cocos display it.
  class RenderTexture final : public CCTexture2D
  {
  public:
    RenderTexture(GLuint name, unsigned width, unsigned height)
    {
      m_uName = name;
      m_uPixelsWide = width;
      m_uPixelsHigh = height;
      m_tContentSize = CCSize(width, height);
      m_ePixelFormat = kCCTexture2DPixelFormat_RGBA8888;
      m_fMaxS = m_fMaxT = 1;
      m_bHasPremultipliedAlpha = true;
      m_bHasMipmaps = false;
    }
    ~RenderTexture() override { m_uName = 0; }
  };
  // A click is tested against her alpha in squares this many pixels wide
  constexpr unsigned MASK_STEP = 8;
  // Opaque enough to count as her
  constexpr uint8_t MASK_SOLID = 128;
} // namespace

// One worker thread decoding the pages asked for one after another, nearest first, with Windows' own PNG decoder
// (WIC), elsewhere stb_image: no cocos object, cache or file utility is touched off the main thread. It stops while two finished pages
// wait to be taken, so the CPU holds at most three pages' pixels (two finished, one being decoded: about 24 MiB).
// The thread owns this state with a shared_ptr and never sees the node: once stopped, it drops what it decoded and
// ends by itself
class MitaPageLoader
{
public:
  static constexpr size_t MAX_READY = 2;

  // Every page is decoded into a slot of this size
  static std::shared_ptr<MitaPageLoader> start(std::vector<std::filesystem::path> pngs, unsigned slotWidth,
                                               unsigned slotHeight)
  {
    auto loader = std::make_shared<MitaPageLoader>();
    loader->m_pngs = std::move(pngs);
    loader->m_slotWidth = slotWidth;
    loader->m_slotHeight = slotHeight;
    std::thread([loader] { loader->run(); }).detach();
    return loader;
  }

  // The pages to decode, nearest first: they replace the ones not started yet. Pages being decoded or waiting
  // aren't decoded again
  void want(std::vector<int> const& atlases)
  {
    std::lock_guard lock(m_mutex);
    if (m_stopped)
      return;
    m_wanted.clear();
    for (auto atlas : atlases)
    {
      if (atlas < 0 || atlas >= static_cast<int>(m_pngs.size()) || atlas == m_decoding || this->isReady(atlas) ||
          std::find(m_wanted.begin(), m_wanted.end(), atlas) != m_wanted.end())
        continue;
      m_wanted.push_back(atlas);
    }
    if (!m_wanted.empty())
      m_wake.notify_one();
  }

  // The pages decoded and waiting, without taking them
  std::vector<int> readyAtlases()
  {
    std::lock_guard lock(m_mutex);
    std::vector<int> atlases;
    for (auto const& page : m_ready)
      atlases.push_back(page.atlas);
    return atlases;
  }

  // Taking one makes room for the next decode
  std::optional<MitaDecodedPage> take(int atlas)
  {
    std::lock_guard lock(m_mutex);
    auto it = std::find_if(m_ready.begin(), m_ready.end(), [&](auto const& page) { return page.atlas == atlas; });
    if (it == m_ready.end())
      return std::nullopt;
    std::optional<MitaDecodedPage> page(std::move(*it));
    m_ready.erase(it);
    m_wake.notify_one();
    return page;
  }

  void stop()
  {
    std::lock_guard lock(m_mutex);
    m_stopped = true;
    m_wanted.clear();
    m_ready.clear();
    m_wake.notify_one();
  }

private:
  // The file's straight-alpha RGBA in page.pixels, its rows as long as the slot's (the rest left transparent), and
  // its size; false when it can't be read or is bigger than a slot
  bool readPixels(std::filesystem::path const& path, MitaDecodedPage& page) const
  {
    auto stride = m_slotWidth * 4;
#ifdef GEODE_IS_WINDOWS
    ComPtr<IWICBitmapDecoder> decoder;
    ComPtr<IWICBitmapFrameDecode> frame;
    ComPtr<IWICBitmapSource> rgba;
    if (!m_factory ||
        FAILED(m_factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
                                                    WICDecodeMetadataCacheOnDemand, &decoder)) ||
        FAILED(decoder->GetFrame(0, &frame)) ||
        // Straight alpha, as the file has it
        FAILED(WICConvertBitmapSource(GUID_WICPixelFormat32bppRGBA, frame.Get(), &rgba)) ||
        FAILED(rgba->GetSize(&page.width, &page.height)) || page.width == 0 || page.height == 0 ||
        page.width > m_slotWidth || page.height > m_slotHeight)
      return false;
    page.pixels.assign(static_cast<size_t>(stride) * m_slotHeight, 0);
    if (FAILED(rgba->CopyPixels(nullptr, stride, static_cast<UINT>(page.pixels.size()), page.pixels.data())))
    {
      page.pixels.clear();
      return false;
    }
    return true;
#else
    int width = 0;
    int height = 0;
    int channels = 0;
    // Straight alpha, as the file has it
    std::unique_ptr<stbi_uc, void (*)(void*)> data(stbi_load(string::pathToString(path).c_str(), &width, &height, &channels, 4),
                                                   stbi_image_free);
    if (!data || width <= 0 || height <= 0 || static_cast<unsigned>(width) > m_slotWidth ||
        static_cast<unsigned>(height) > m_slotHeight)
      return false;
    page.width = static_cast<unsigned>(width);
    page.height = static_cast<unsigned>(height);
    page.pixels.assign(static_cast<size_t>(stride) * m_slotHeight, 0);
    for (unsigned y = 0; y < page.height; y++)
      std::memcpy(page.pixels.data() + static_cast<size_t>(y) * stride,
                  data.get() + static_cast<size_t>(y) * page.width * 4, page.width * 4);
    return true;
#endif
  }

  MitaDecodedPage decode(std::filesystem::path const& path, int atlas) const
  {
    MitaDecodedPage page;
    page.atlas = atlas;
    auto began = std::chrono::steady_clock::now();
    if (!this->readPixels(path, page))
      return page;
    auto stride = m_slotWidth * 4;
    // The mask, made here off the main thread
    page.maskWidth = (page.width + MASK_STEP - 1) / MASK_STEP;
    page.maskHeight = (page.height + MASK_STEP - 1) / MASK_STEP;
    page.mask.assign(static_cast<size_t>(page.maskWidth) * page.maskHeight, 0);
    for (unsigned y = 0; y < page.height; y++)
    {
      auto row = page.pixels.data() + static_cast<size_t>(y) * stride;
      auto maskRow = page.mask.data() + static_cast<size_t>(y / MASK_STEP) * page.maskWidth;
      for (unsigned x = 0; x < page.width; x++)
      {
        auto alpha = row[x * 4 + 3];
        auto& cell = maskRow[x / MASK_STEP];
        if (alpha > cell)
          cell = alpha;
      }
    }
    page.ok = true;
    page.milliseconds =
      std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - began).count();
    return page;
  }

  void run()
  {
#ifdef GEODE_IS_WINDOWS
    auto com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_factory));
#endif
    {
      while (true)
      {
        int atlas = -1;
        {
          std::unique_lock lock(m_mutex);
          m_wake.wait(lock, [this] { return m_stopped || (!m_wanted.empty() && m_ready.size() < MAX_READY); });
          if (m_stopped)
            break;
          atlas = m_wanted.front();
          m_wanted.erase(m_wanted.begin());
          m_decoding = atlas;
        }
        auto page = decode(m_pngs[atlas], atlas);
        std::lock_guard lock(m_mutex);
        m_decoding = -1;
        if (m_stopped)
          break;
        m_ready.push_back(std::move(page));
      }
    }
#ifdef GEODE_IS_WINDOWS
    m_factory.Reset();
    if (SUCCEEDED(com))
      CoUninitialize();
#endif
  }

  bool isReady(int atlas) const
  {
    return std::any_of(m_ready.begin(), m_ready.end(), [&](auto const& page) { return page.atlas == atlas; });
  }

  std::mutex m_mutex;
  std::condition_variable m_wake;
#ifdef GEODE_IS_WINDOWS
  // Made and released on the worker thread
  ComPtr<IWICImagingFactory> m_factory;
#endif
  std::vector<std::filesystem::path> m_pngs;
  unsigned m_slotWidth = 0;
  unsigned m_slotHeight = 0;
  std::vector<int> m_wanted;
  int m_decoding = -1;
  std::vector<MitaDecodedPage> m_ready;
  bool m_stopped = false;
};

// ! --- The character --- !

namespace
{
  // The close-up set (resources/mita-closeup): 127 frames of 960×1080, two a page
  constexpr char const* MANIFEST = "mita-closeup-manifest.json";
  // The measured load time weighs this much against the ones before
  constexpr double LOAD_SMOOTHING = 0.3;
  // A request looks ahead by this much more than a load takes
  constexpr double LOAD_MARGIN = 1.5;

  // Her outline: the usual sprite vertex shader, and a fragment shader that adds a line where a transparent pixel
  // has a drawn one within the line's width. Her own pixels are left as they are; nothing shows around the canvas'
  // rectangle, which is transparent. cocos 2.2 declares only CC_MVPMatrix and the like; the texture is ours to declare
  constexpr char const* OUTLINE_VERTEX = R"(
attribute vec4 a_position;
attribute vec2 a_texCoord;
attribute vec4 a_color;
#ifdef GL_ES
varying lowp vec4 v_fragmentColor;
varying mediump vec2 v_texCoord;
#else
varying vec4 v_fragmentColor;
varying vec2 v_texCoord;
#endif
void main()
{
  gl_Position = CC_MVPMatrix * a_position;
  v_fragmentColor = a_color;
  v_texCoord = a_texCoord;
}
)";

  constexpr char const* OUTLINE_FRAGMENT = R"(
#ifdef GL_ES
precision mediump float;
#endif
varying vec4 v_fragmentColor;
varying vec2 v_texCoord;
uniform sampler2D CC_Texture0;
uniform vec2 u_texel;
uniform vec4 u_outline;
uniform float u_premult;
void main()
{
  vec4 color = texture2D(CC_Texture0, v_texCoord);
  // Inside her: nothing to add, and no need to look around
  if (color.a > 0.99)
  {
    gl_FragColor = color * v_fragmentColor;
    return;
  }
  // Two rings of eight, the inner one turned half a step: sixteen directions, so a wide line stays smooth
  float around = 0.0;
  for (int i = 0; i < 8; i++)
  {
    float angle = 0.7853982 * float(i);
    around = max(around, texture2D(CC_Texture0, v_texCoord + vec2(cos(angle), sin(angle)) * u_texel).a);
    float inner = angle + 0.3926991;
    around = max(around, texture2D(CC_Texture0, v_texCoord + vec2(cos(inner), sin(inner)) * u_texel * 0.55).a);
  }
  float edge = clamp(around - color.a, 0.0, 1.0) * u_outline.a;
  float alpha = color.a + edge * (1.0 - color.a);
  vec3 rgb = mix(color.rgb * color.a, color.rgb, u_premult) + u_outline.rgb * edge * (1.0 - color.a);
  if (u_premult < 0.5) rgb = alpha > 0.0 ? rgb / alpha : vec3(0.0);
  gl_FragColor = vec4(rgb, alpha) * v_fragmentColor;
}
)";

  // Each problem once a session, however many times the menu opens
  void logOnce(std::string const& message)
  {
    static std::set<std::string> s_logged;
    if (s_logged.insert(message).second)
      log::error("Mita: {}", message);
  }
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
  this->release3D();
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
  // Where her drawing starts in the canvas (all frames together); the canvas' edge when it isn't said
  m_alphaLeft = static_cast<float>(manifest["canvas"]["alphaBoundsUnionTopLeft"][0].asDouble().unwrapOr(0));
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
    m_slotSize.first = std::max(m_slotSize.first, static_cast<unsigned>(atlas.width));
    m_slotSize.second = std::max(m_slotSize.second, static_cast<unsigned>(atlas.height));
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
  if (!CCNode::init())
    return false;
  m_modelPath = Mod::get()->getResourcesDir() / "mita.glb";
  std::error_code error;
#if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_ANDROID)
  m_use3D = std::filesystem::is_regular_file(m_modelPath, error);
#else
  // No 3D renderer for this platform yet: the atlases
  m_use3D = false;
#endif
  log::info("Mita: {} ({}{})", m_use3D ? "the 3D model" : "the atlases", string::pathToString(m_modelPath),
            error ? ", " + error.message() : std::string());
  if (m_use3D)
  {
    m_canvas = CCSize(960, 1080);
    m_reference = CCSize(569, 320);
    m_alphaLeft = 80;
    this->makeOutline();
    this->setContentSize(CCDirector::get()->getWinSize());
    return true;
  }
  if (!this->readManifest())
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
  this->makeOutline();
  this->setContentSize(CCDirector::get()->getWinSize());
  return true;
}

void MSMita::makeOutline()
{
  auto const& theme = Theme::get();
  if (!theme.mitaOutlineEnabled || theme.mitaOutlineOpacity == 0 || theme.mitaOutlineWidth <= 0)
    return;
  // Hers alone: not in the shader cache, not on any other sprite. Made once a session
  auto program = ms::sharedProgram(OUTLINE_VERTEX, OUTLINE_FRAGMENT, "Mita's outline shader (she's drawn without it)");
  if (!program)
    return;
  m_outlineGeneration = ms::programGeneration();
  m_outlineTexel = program->getUniformLocationForName("u_texel");
  m_outlineColor = program->getUniformLocationForName("u_outline");
  m_outlinePremult = program->getUniformLocationForName("u_premult");
  m_outline = program;
}

void MSMita::updateOutline(CCTexture2D* texture)
{
  if (!m_outline || !m_sprite || !texture)
    return;
  // Made again in a new GL context: its uniforms found and set again
  if (m_outlineGeneration != ms::programGeneration())
  {
    m_outlineGeneration = ms::programGeneration();
    m_outlineTexel = m_outline->getUniformLocationForName("u_texel");
    m_outlineColor = m_outline->getUniformLocationForName("u_outline");
    m_outlinePremult = m_outline->getUniformLocationForName("u_premult");
    m_outlineFor = nullptr;
  }
  if (texture == m_outlineFor)
    return;
  auto const& theme = Theme::get();
  m_outlineFor = texture;
  m_sprite->setShaderProgram(m_outline);
  m_outline->use();
  m_outline->setUniformLocationWith1f(m_outlinePremult, texture->hasPremultipliedAlpha() ? 1.f : 0.f);
  m_outline->setUniformLocationWith2f(m_outlineTexel, theme.mitaOutlineWidth / texture->getPixelsWide(),
                                      theme.mitaOutlineWidth / texture->getPixelsHigh());
  auto color = theme.mitaOutlineColor;
  m_outline->setUniformLocationWith4f(m_outlineColor, color.r / 255.f, color.g / 255.f, color.b / 255.f,
                                      theme.mitaOutlineOpacity / 255.f);
}

void MSMita::visit()
{
  // Two screens can show different texture sizes during a transition; uniforms are per draw.
  if (m_sprite && m_outline)
  {
    m_outlineFor = nullptr;
    this->updateOutline(m_sprite->getTexture());
  }
  CCNode::visit();
}

void MSMita::onEnter()
{
  CCNode::onEnter();
  this->start();
  // After the menus (a button over her takes its click); never swallowing
  CCTouchDispatcher::get()->addTargetedDelegate(this, 10, false);
}

void MSMita::onExit()
{
  // Nothing of hers outlives the menu being on screen: the pages are loaded again if it comes back
  this->stop();
  CCTouchDispatcher::get()->removeDelegate(this);
  CCNode::onExit();
}

// ! --- A click on her --- !

bool MSMita::isDrawnAt(CCPoint world)
{
  if (m_renderer && m_sprite)
  {
    auto local = m_sprite->convertToNodeSpace(world);
    auto size = m_sprite->getContentSize();
    return size.width > 0 && size.height > 0 && m_renderer->hit(local.x / size.width, local.y / size.height);
  }
  if (!m_sprite || m_shownFrame < 0)
    return false;
  auto const& frame = m_frames[m_shownFrame];
  auto page = this->resident(frame.atlas);
  if (!page || page->mask.empty())
    return false;
  // The point in her frame, then in the page's pixels (the frame's rect is in pixels from the page's top left)
  auto local = m_sprite->convertToNodeSpace(world);
  auto size = m_sprite->getContentSize();
  if (size.width <= 0 || size.height <= 0 || local.x < 0 || local.y < 0 || local.x >= size.width ||
      local.y >= size.height)
    return false;
  auto x = frame.rect.origin.x + local.x / size.width * frame.rect.size.width;
  auto y = frame.rect.origin.y + (1 - local.y / size.height) * frame.rect.size.height;
  auto column = static_cast<unsigned>(std::max(x, 0.f)) / MASK_STEP;
  auto row = static_cast<unsigned>(std::max(y, 0.f)) / MASK_STEP;
  if (column >= page->maskWidth || row >= page->maskHeight)
    return false;
  return page->mask[static_cast<size_t>(row) * page->maskWidth + column] >= MASK_SOLID;
}

bool MSMita::ccTouchBegan(CCTouch* touch, CCEvent*)
{
  auto const& theme = Theme::get();
  if (!theme.mitaSparkEnabled || m_failed)
    return false;
  // Shown, and no popup over her
  for (CCNode* node = this; node; node = node->getParent())
  {
    if (!node->isVisible())
      return false;
  }
  if (ms::coveredBy(this))
    return false;
  auto world = touch->getLocation();
  if (!this->isDrawnAt(world))
    return false;
  this->spark(world);
  // MiSide's own click on her, with the sparks, as loud as GD's effects
  ms::playSound(theme.mitaSparkSound);
  if (m_renderer && m_sprite && m_swatTime < 0 && m_swatCooldown <= 0)
  {
    auto local = m_sprite->convertToNodeSpace(world);
    auto size = m_sprite->getContentSize();
    // A face-sized ellipse inside the silhouette, excluding the torso and hair above her forehead.
    float originalU = (local.x / size.width - (1 - mita3d::OriginalCanvasFraction)) / mita3d::OriginalCanvasFraction;
    float x = (originalU - 0.53f) / 0.22f;
    float y = (local.y / size.height - 0.49f) / 0.18f;
    if (x * x + y * y <= 1)
    {
      auto now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
      std::erase_if(m_faceClicks, [now](double at) { return now - at > 1.2; });
      m_faceClicks.push_back(now);
      if (m_faceClicks.size() >= 4)
      {
        m_faceClicks.clear();
        m_swatTime = 0;
      }
    }
  }
  return false;
}

void MSMita::spark(CCPoint world)
{
  auto const& theme = Theme::get();
  if (!ms::resourceExists(theme.glowSprite))
    return;
  auto duration = std::max(theme.mitaSparkDuration, 0.05f);
  auto at = this->convertToNodeSpace(world);
  static std::mt19937 s_random{std::random_device{}()};
  std::uniform_real_distribution<float> unit(0.f, 1.f);

  // A soft white light added to her (on her pink skin it burns to white, as in MiSide)
  auto light = [&](float size, float stretch, float angle) -> CCSprite* {
    auto sprite = CCSprite::create(theme.glowSprite.c_str());
    if (!sprite)
      return nullptr;
    auto own = sprite->getContentSize();
    if (own.width <= 0)
      return nullptr;
    sprite->setScaleX(size * stretch / own.width);
    sprite->setScaleY(size / stretch / own.width);
    sprite->setRotation(angle);
    sprite->setColor(theme.mitaSparkColor);
    sprite->setOpacity(theme.mitaSparkOpacity);
    auto texture = sprite->getTexture();
    auto premultiplied = texture && texture->hasPremultipliedAlpha();
    sprite->setBlendFunc({static_cast<GLenum>(premultiplied ? GL_ONE : GL_SRC_ALPHA), GL_ONE});
    return sprite;
  };

  // The flash: at once, a little bigger, gone in the first part
  if (auto flash = light(theme.mitaSparkFlashSize, 1.15f, unit(s_random) * 180))
  {
    auto node = CCNode::create();
    node->setPosition(at);
    node->addChild(flash);
    node->setScale(0.75f);
    this->addChild(node, 10);
    auto time = duration * 0.6f;
    node->runAction(CCSequence::createWithTwoActions(CCEaseOut::create(CCScaleTo::create(time, 1.15f), 2),
                                                     CCRemoveSelf::create()));
    flash->runAction(CCEaseIn::create(CCFadeOut::create(time), 2));
  }

  // The blobs: each two lights a little apart, so it isn't round; out from the click, growing then shrinking away
  auto count = std::clamp(static_cast<int>(theme.mitaSparkCount), 0, 16);
  auto turn = unit(s_random) * 6.2831853f;
  for (int i = 0; i < count; i++)
  {
    auto node = CCNode::create();
    auto size = theme.mitaSparkSize * (0.6f + 0.8f * unit(s_random));
    for (int part = 0; part < 2; part++)
    {
      auto sprite = light(size * (part == 0 ? 1.f : 0.7f), 1.2f + unit(s_random) * 0.6f, unit(s_random) * 180);
      if (!sprite)
        continue;
      sprite->setPosition({(unit(s_random) - 0.5f) * size * 0.5f, (unit(s_random) - 0.5f) * size * 0.4f});
      sprite->runAction(CCSequence::createWithTwoActions(CCDelayTime::create(duration * 0.45f),
                                                         CCFadeOut::create(duration * 0.55f)));
      node->addChild(sprite);
    }
    // Spread around the click, unevenly
    auto angle = turn + (i + unit(s_random) * 0.7f) * 6.2831853f / std::max(count, 1);
    auto reach = theme.mitaSparkSpread * (0.5f + 0.5f * unit(s_random));
    CCPoint away{std::cos(angle) * reach, std::sin(angle) * reach * 0.8f};
    node->setPosition(at);
    node->setScale(0.4f);
    this->addChild(node, 10);
    auto grow = CCSequence::createWithTwoActions(CCScaleTo::create(duration * 0.35f, 1),
                                                 CCScaleTo::create(duration * 0.65f, 0.3f));
    node->runAction(CCSequence::createWithTwoActions(
      CCSpawn::createWithTwoActions(CCEaseOut::create(CCMoveBy::create(duration, away), 2.2f), grow),
      CCRemoveSelf::create()));
  }
}

bool MSMita::start3D()
{
  if (m_renderer) return true;
  try
  {
    auto started = std::chrono::steady_clock::now();
    m_contextGeneration = ms::programGeneration();
    m_renderer = std::make_unique<mita3d::Renderer>(m_modelPath);
    m_renderWait = 0;
    this->update3D(0);
    log::info("3D Mita loaded in {:.1f} ms; OpenGL {}",
              std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count(),
              reinterpret_cast<char const*>(glGetString(GL_VERSION)));
    return true;
  }
  catch (std::exception const& e)
  {
    logOnce(fmt::format("3D unavailable, using atlases: {}", e.what()));
    if (m_sprite) { m_sprite->removeFromParentAndCleanup(true); m_sprite = nullptr; }
    this->release3D();
    return false;
  }
}

void MSMita::release3D()
{
  m_renderTexture = nullptr;
  if (m_renderer)
  {
    if (m_contextGeneration != ms::programGeneration())
      m_renderer->abandonContext();
    else
      // Only the output texture enters cocos' cache. Unbind it before raw GL deletion;
      // ccGLInvalidateStateCache also frees the scene's matrix stacks.
      ccGLBindTexture2D(0);
    m_renderer.reset();
  }
  m_renderHeight = 0;
  m_outlineFor = nullptr;
}

void MSMita::update3D(float dt)
{
  if (!m_renderer || m_failed) return;
  if (m_contextGeneration != ms::programGeneration())
  {
    if (m_sprite) { m_sprite->removeFromParentAndCleanup(true); m_sprite = nullptr; }
    this->release3D();
    if (!this->start3D()) this->fail("3D context could not be restored");
    return;
  }
  dt = std::clamp(dt, 0.f, 0.1f);
  m_elapsed += dt;
  if (m_swatTime >= 0)
  {
    m_swatTime += dt;
    if (m_swatTime >= mita3d::SwatDuration)
    {
      m_swatTime = -1;
      m_swatCooldown = 3.f;
    }
  }
  else
    m_swatCooldown = std::max(0.f, m_swatCooldown - dt);
  bool exiting = m_exitHovered || m_exitConfirmation;
  m_expression += ((exiting ? 1.f : 0.f) - m_expression) * (1 - std::exp(-dt / 0.18f));
  auto win = CCDirector::get()->getWinSize();
  const float towardViewer = -mita3d::ModelYaw / mita3d::GazeYawRange;
  CCPoint aim(exiting ? towardViewer : 0.f, 0);
  bool trackingMenu = !exiting && m_menuGazeTarget && m_sprite && !ms::coveredBy(this) && win.width > 0 && win.height > 0;
  if (trackingMenu)
  {
    auto size = m_sprite->getContentSize();
    auto eyes = m_sprite->convertToWorldSpace({size.width * mita3d::CanvasAnchorX, size.height * 0.59f});
    auto delta = *m_menuGazeTarget - eyes;
    aim = CCPoint(std::clamp(delta.x / (win.width * 0.6f) + towardViewer, -1.f, 1.f),
                  std::clamp(delta.y / (win.height * 0.42f), -1.f, 1.f));
  }
  if (trackingMenu) {
    m_lastGazeAim = aim;
    m_gazeHold = 0.65f;
  } else {
    m_gazeHold = exiting ? 0.f : std::max(0.f, m_gazeHold - dt);
  }
  auto heldAim = m_gazeHold > 0 ? m_lastGazeAim : aim;
  if (m_swatTime >= 0)
  {
    auto smooth = [](float value) {
      auto t = std::clamp(value, 0.f, 1.f);
      return t * t * (3 - 2 * t);
    };
    auto towardViewer = smooth(m_swatTime / 0.35f) * smooth((mita3d::SwatDuration - m_swatTime) / 0.5f);
    heldAim = heldAim * (1 - towardViewer);
  }
  m_gaze = m_gaze + (heldAim - m_gaze) * (1 - std::exp(-dt / 0.025f));
  m_headGaze = m_headGaze + (heldAim - m_headGaze) * (1 - std::exp(-dt / 0.38f));
  auto frame = CCEGLView::sharedOpenGLView()->getFrameSize();
  int height = std::clamp(static_cast<int>(frame.height), 540, 1440);
  int width = static_cast<int>(height * mita3d::CanvasAspect);
  m_renderWait += dt;
  if (m_sprite && height == m_renderHeight && m_renderWait < 1.f / 60.f) return;
  m_renderWait = std::fmod(m_renderWait, 1.f / 60.f);
  m_renderer->render(width, height, m_elapsed, m_expression, {m_gaze.x, m_gaze.y}, {m_headGaze.x, m_headGaze.y}, m_swatTime);
  if (!m_sprite || height != m_renderHeight)
  {
    auto texture = new RenderTexture(m_renderer->texture(), width, height);
    m_renderTexture = texture;
    texture->release();
    bool created = !m_sprite;
    if (created)
    {
      m_sprite = CCSprite::createWithTexture(texture);
      if (!m_sprite) throw std::runtime_error("Cannot create 3D Mita sprite");
      m_sprite->setAnchorPoint({mita3d::CanvasAnchorX, 0});
      this->addChild(m_sprite);
    }
    else
    {
      m_sprite->setTexture(texture);
      m_sprite->setTextureRect({0, 0, width / CC_CONTENT_SCALE_FACTOR(), height / CC_CONTENT_SCALE_FACTOR()});
    }
    m_sprite->setFlipY(true);
    m_sprite->setBlendFunc({GL_ONE, GL_ONE_MINUS_SRC_ALPHA});
    m_sprite->setOpacityModifyRGB(true);
    m_renderHeight = height;
    m_outlineFor = nullptr;
    this->layout();
    if (created && Theme::get().introDuration > 0)
    {
      m_sprite->setOpacity(0);
      m_sprite->runAction(CCFadeTo::create(Theme::get().introDuration, 255));
    }
  }
  if (win != m_laidOutFor) this->layout();
  m_sprite->setPosition(m_home);
  this->updateOutline(m_renderTexture);
}

void MSMita::start()
{
  log::debug("Mita: starting ({}{})", m_use3D ? "3D" : "atlases", m_failed ? ", failed before" : "");
  if (m_use3D && !m_failed)
  {
    if (this->start3D())
    {
      this->scheduleUpdate();
      return;
    }
    // Keep the installed 2D assets usable if this GPU or model fails.
    m_use3D = false;
    if (!this->readManifest()) { m_failed = true; return; }
    m_plan = std::make_unique<MitaPlan>(static_cast<int>(m_atlases.size()));
  }
  if (m_failed || m_loader)
    return;
  std::vector<std::filesystem::path> pngs;
  for (auto const& atlas : m_atlases)
    pngs.push_back(atlas.png);
  m_loader = MitaPageLoader::start(std::move(pngs), m_slotSize.first, m_slotSize.second);
  this->scheduleUpdate();
  // The page for where her loop is now, right away
  auto index = MitaPlan::frameAt(m_elapsed, m_fps, static_cast<int>(m_frames.size()));
  m_loader->want({m_frames[index].atlas});
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
  this->release3D();
  m_shownFrame = -1;
  m_outlineFor = nullptr;
  if (m_plan)
    this->freePages(m_plan->clear());
  // The textures themselves, only now: nothing of hers stays on the GPU once the menu is gone
  for (auto const& page : m_pages)
  {
    if (page->atlas >= 0)
      log::warn("Mita: atlas {} left after freeing them all", page->atlas);
    if (auto texture = page->texture.data(); texture && texture->retainCount() != 1)
      log::warn("Mita: a texture slot is still held {} more times, it isn't freed", texture->retainCount() - 1);
  }
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
  if (atlas < 0)
    return nullptr;
  for (auto const& page : m_pages)
  {
    if (page->atlas == atlas)
      return page.get();
  }
  return nullptr;
}

void MSMita::freePages(std::vector<int> const& atlases)
{
  for (auto atlas : atlases)
  {
    auto page = this->resident(atlas);
    if (!page)
      continue;
    // The slot is free for the next page: its texture stays, to be filled again rather than deleted and made anew
    // (deleting and making an 8 MiB texture every 83 ms stalls the driver)
    page->frames.clear();
    page->atlas = -1;
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
  // Never a fourth page on the GPU, even for a moment: room is made first, never from the shown page
  this->freePages(m_plan->makeRoom(wanted));
  auto slot = std::find_if(m_pages.begin(), m_pages.end(), [](auto const& page) { return page->atlas < 0; });
  if (slot == m_pages.end() && m_pages.size() >= MitaPlan::RESIDENT)
  {
    log::warn("Mita: no room for atlas {}, it's skipped", decoded.atlas);
    return true;
  }

  auto began = std::chrono::steady_clock::now();
  auto [slotWidth, slotHeight] = m_slotSize;
  Page* target = nullptr;
  if (slot != m_pages.end())
  {
    // A free slot: its texture filled again in place, the same size and format
    target = slot->get();
    ccGLBindTexture2D(target->texture->getName());
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, slotWidth, slotHeight, GL_RGBA, GL_UNSIGNED_BYTE, decoded.pixels.data());
  }
  else
  {
    // One of the three slots, made once while she's on screen
    auto texture = new CCTexture2D();
    // RGBA8888, straight alpha (a texture made from raw data isn't premultiplied)
    if (!texture->initWithData(decoded.pixels.data(), kCCTexture2DPixelFormat_RGBA8888, slotWidth, slotHeight,
                               CCSize(slotWidth, slotHeight)))
    {
      texture->release();
      this->fail(fmt::format("can't upload {}", atlas.png.filename()));
      return false;
    }
    ccTexParams params{GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE};
    texture->setTexParameters(&params);
    auto owned = std::make_unique<Page>();
    owned->texture = texture;
    texture->release();
    owned->bytes = decoded.pixels.size();
    target = owned.get();
    m_pages.push_back(std::move(owned));
  }
  auto& page = *target;
  page.atlas = decoded.atlas;
  page.mask = std::move(decoded.mask);
  page.maskWidth = decoded.maskWidth;
  page.maskHeight = decoded.maskHeight;
  auto texture = page.texture.data();
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
  m_plan->uploaded(decoded.atlas);
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

  // Held to the right edge, as on the reference screen; her drawing (not the canvas' transparent margin) never
  // over the menu's buttons
  auto x = winSize.width - (m_reference.width - theme.mitaX) * m_unit;
  auto keepClear = theme.columnLeft + theme.rowWidth + theme.columnClearance;
  auto layoutWidth = m_use3D ? width * mita3d::OriginalCanvasFraction : width;
  auto drawnLeft = (m_canvas.width / 2 - m_alphaLeft) * layoutWidth / m_canvas.width;
  x = std::max(x, keepClear + drawnLeft);
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
  this->updateOutline(page->texture);
  m_shownFrame = index;
  // The page before is done with
  this->freePages(m_plan->showed(frame.atlas));
}

void MSMita::checkSprite()
{
  // What cocos actually made of it, once each time the menu opens: the frame's rect in the texture's pixels, the
  // size in points, and the size and place on the screen. Only a size that isn't the one meant is logged
  auto const& theme = Theme::get();
  auto scale = CC_CONTENT_SCALE_FACTOR();
  auto rect = m_sprite->getTextureRect();
  CCRect pixels{rect.origin.x * scale, rect.origin.y * scale, rect.size.width * scale, rect.size.height * scale};
  auto content = m_sprite->getContentSize();
  auto box = CCRectApplyAffineTransform(CCRect{CCPointZero, content}, m_sprite->nodeToWorldTransform());
  auto expectedHeight = m_canvas.height * theme.mitaScale * m_unit;
  auto expectedWidth = m_canvas.width * theme.mitaScale * m_unit;
  if (std::abs(pixels.size.width - m_canvas.width) > 0.5f || std::abs(pixels.size.height - m_canvas.height) > 0.5f ||
      std::abs(box.size.height - expectedHeight) > 0.5f || std::abs(box.size.width - expectedWidth) > 0.5f)
    log::warn("Mita: she's drawn {:.3f}x{:.3f} from a {}x{} px rect, {:.3f}x{:.3f} from {}x{} was meant", box.size.width,
              box.size.height, pixels.size.width, pixels.size.height, expectedWidth, expectedHeight, m_canvas.width,
              m_canvas.height);
}

void MSMita::update(float dt)
{
  if (m_use3D)
  {
    for (CCNode* node = this; node; node = node->getParent())
      if (!node->isVisible()) return;
    try { this->update3D(dt); }
    catch (std::exception const& e) { this->fail(fmt::format("3D render failed: {}", e.what())); }
    return;
  }
  if (m_failed || !m_loader)
    return;
  // Under a screen that covers the menu (a parent hidden): she waits where she is, her pages kept
  for (CCNode* node = this; node; node = node->getParent())
  {
    if (!node->isVisible())
      return;
  }
  auto const& theme = Theme::get();
  auto count = static_cast<int>(m_frames.size());
  // From the time that passed: the same speed at any frame rate, and a late frame jumps to where it should be. Her
  // clock starts with her first frame shown, so she doesn't start by skipping the frames her first page took
  if (m_sprite)
    m_elapsed = MitaPlan::advance(m_elapsed, dt, m_fps, count);
  auto index = MitaPlan::frameAt(m_elapsed, m_fps, count);
  // A still frame, to check one by one
  if (theme.mitaFreezeFrame >= 0)
    index = std::clamp(static_cast<int>(theme.mitaFreezeFrame), 0, count - 1);
  auto atlas = m_frames[index].atlas;

  // The page for now first, if it's there: switching frees the page before, making room for the next
  if (this->resident(atlas))
    this->show(index);

  // The finished pages: on the GPU if they're for now or the next two, left waiting decoded if they're for soon,
  // else dropped
  for (auto ready : m_loader->readyAtlases())
  {
    auto use = m_plan->decide(ready, atlas);
    if (use == MitaPlan::Use::Wait)
      continue;
    auto decoded = m_loader->take(ready);
    if (!decoded)
      continue;
    if (!decoded->ok)
    {
      this->fail(fmt::format("can't decode {}", m_atlases[decoded->atlas].png.filename()));
      return;
    }
    if (use == MitaPlan::Use::Upload && !this->upload(*decoded, atlas))
      return;
  }

  // Just uploaded, or not there yet: then the last frame stays until it is, and the loop catches up with the time
  if (this->resident(atlas))
    this->show(index);
  // Where playback will be once the next load ends; the pages from there on, nearest first
  auto later = MitaPlan::advance(m_elapsed, m_loadSeconds * LOAD_MARGIN, m_fps, count);
  auto predicted = m_frames[MitaPlan::frameAt(later, m_fps, count)].atlas;
  m_loader->want(m_plan->toRequest(atlas, predicted));

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
