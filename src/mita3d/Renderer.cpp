// Desktop OpenGL 3.0 through GLEW and Windows' WIC: Windows only for now. Elsewhere the renderer can't be made,
// and MSMita shows the atlases instead
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#include <GL/glew.h>
#include <wincodec.h>
#include <wrl/client.h>
#include "Renderer.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>

using Microsoft::WRL::ComPtr;
namespace mita3d {
namespace {
// All GL calls here bypass cocos' caches; restore their exact incoming values.
struct State {
    GLint program, vao, buffer, readFbo, drawFbo, rbo, activeTexture, texture0, viewport[4];
    GLint depthFunc, pack, unpack, polygon[2], packBuffer, unpackBuffer;
    GLint packRow, packSkipRows, packSkipPixels, unpackRow, unpackSkipRows, unpackSkipPixels;
    GLboolean depthMask, colorMask[4];
    GLfloat clearColor[4]; GLdouble clearDepth, depthRange[2];
    static constexpr GLenum caps[] = {GL_BLEND, GL_DEPTH_TEST, GL_CULL_FACE, GL_SCISSOR_TEST,
        GL_STENCIL_TEST, GL_FRAMEBUFFER_SRGB, GL_MULTISAMPLE, GL_SAMPLE_ALPHA_TO_COVERAGE, GL_RASTERIZER_DISCARD};
    GLboolean enabled[std::size(caps)];
    State() {
        glGetIntegerv(GL_CURRENT_PROGRAM, &program); glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer); glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFbo);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFbo); glGetIntegerv(GL_RENDERBUFFER_BINDING, &rbo);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture); glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture0); glGetIntegerv(GL_VIEWPORT, viewport);
        glGetIntegerv(GL_DEPTH_FUNC, &depthFunc); glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
        glGetBooleanv(GL_COLOR_WRITEMASK, colorMask); glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColor);
        glGetDoublev(GL_DEPTH_CLEAR_VALUE, &clearDepth); glGetDoublev(GL_DEPTH_RANGE, depthRange);
        glGetIntegerv(GL_PACK_ALIGNMENT, &pack); glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpack);
        glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &packBuffer); glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &unpackBuffer);
        glGetIntegerv(GL_PACK_ROW_LENGTH, &packRow); glGetIntegerv(GL_PACK_SKIP_ROWS, &packSkipRows);
        glGetIntegerv(GL_PACK_SKIP_PIXELS, &packSkipPixels); glGetIntegerv(GL_UNPACK_ROW_LENGTH, &unpackRow);
        glGetIntegerv(GL_UNPACK_SKIP_ROWS, &unpackSkipRows); glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &unpackSkipPixels);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0); glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        glPixelStorei(GL_PACK_ROW_LENGTH, 0); glPixelStorei(GL_PACK_SKIP_ROWS, 0); glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0); glPixelStorei(GL_UNPACK_SKIP_ROWS, 0); glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
        glGetIntegerv(GL_POLYGON_MODE, polygon);
        for (size_t i = 0; i < std::size(caps); ++i) enabled[i] = glIsEnabled(caps[i]);
    }
    ~State() {
        glUseProgram(program); glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, readFbo); glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo); glBindTexture(GL_TEXTURE_2D, texture0);
        glActiveTexture(activeTexture); glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        glDepthFunc(depthFunc); glDepthMask(depthMask); glDepthRange(depthRange[0], depthRange[1]);
        glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
        glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]); glClearDepth(clearDepth);
        glPixelStorei(GL_PACK_ALIGNMENT, pack); glPixelStorei(GL_UNPACK_ALIGNMENT, unpack);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, packBuffer); glBindBuffer(GL_PIXEL_UNPACK_BUFFER, unpackBuffer);
        glPixelStorei(GL_PACK_ROW_LENGTH, packRow); glPixelStorei(GL_PACK_SKIP_ROWS, packSkipRows); glPixelStorei(GL_PACK_SKIP_PIXELS, packSkipPixels);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, unpackRow); glPixelStorei(GL_UNPACK_SKIP_ROWS, unpackSkipRows); glPixelStorei(GL_UNPACK_SKIP_PIXELS, unpackSkipPixels);
        glPolygonMode(GL_FRONT, polygon[0]); glPolygonMode(GL_BACK, polygon[1]);
        for (size_t i = 0; i < std::size(caps); ++i) { if (enabled[i]) glEnable(caps[i]); else glDisable(caps[i]); }
    }
};
void checked(HRESULT hr) { if (FAILED(hr)) throw std::runtime_error("Cannot decode embedded Mita PNG"); }
GLuint shader(GLenum type, char const* source) {
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &source, nullptr); glCompileShader(id);
    GLint ok = 0; glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[2048]{}; glGetShaderInfoLog(id, sizeof(log), nullptr, log); glDeleteShader(id); throw std::runtime_error(log); }
    return id;
}
constexpr char VERTEX[] = R"(#version 120
attribute vec3 a_position;
attribute vec2 a_uv;
uniform mat4 u_camera;
varying vec2 v_uv;
void main() { gl_Position = u_camera * vec4(a_position, 1.0); v_uv = a_uv; }
)";
constexpr char FRAGMENT[] = R"(#version 120
uniform sampler2D u_image;
varying vec2 v_uv;
void main() {
    vec4 color = texture2D(u_image, v_uv);
    // The author's materials are opaque, emissive, with painted lighting.
    gl_FragColor = vec4(color.rgb, 1.0);
}
)";
}
struct Renderer::Data {
    Model model;
    GLuint program = 0, vao = 0, vbo = 0, ibo = 0;
    GLuint fbo = 0, resolve = 0, color = 0, depth = 0, output = 0;
    GLint camera = -1;
    int width = 0, height = 0, samples = 1;
    std::vector<GLuint> images;
    explicit Data(std::filesystem::path const& path) : model(path) {}
    ~Data() {
        if (program) glDeleteProgram(program);
        glDeleteTextures(static_cast<GLsizei>(images.size()), images.data());
        glDeleteTextures(1, &output); glDeleteFramebuffers(1, &fbo); glDeleteFramebuffers(1, &resolve);
        glDeleteRenderbuffers(1, &color); glDeleteRenderbuffers(1, &depth);
        glDeleteBuffers(1, &vbo); glDeleteBuffers(1, &ibo); glDeleteVertexArrays(1, &vao);
    }
};
Renderer::Renderer(std::filesystem::path const& path) {
    if (!GLEW_VERSION_3_0) throw std::runtime_error("3D Mita requires OpenGL 3.0 (FBO, VAO, MSAA resolve)");
    State state;
    m = std::make_unique<Data>(path);
    GLuint vs = shader(GL_VERTEX_SHADER, VERTEX);
    GLuint fs = 0;
    try { fs = shader(GL_FRAGMENT_SHADER, FRAGMENT); } catch (...) { glDeleteShader(vs); throw; }
    m->program = glCreateProgram();
    glAttachShader(m->program, vs); glAttachShader(m->program, fs);
    glBindAttribLocation(m->program, 0, "a_position"); glBindAttribLocation(m->program, 1, "a_uv");
    glLinkProgram(m->program); glDeleteShader(vs); glDeleteShader(fs);
    GLint ok; glGetProgramiv(m->program, GL_LINK_STATUS, &ok);
    if (!ok) throw std::runtime_error("Cannot link Mita shader");
    m->camera = glGetUniformLocation(m->program, "u_camera");
    glUseProgram(m->program); glUniform1i(glGetUniformLocation(m->program, "u_image"), 0);
    glGenVertexArrays(1, &m->vao); glBindVertexArray(m->vao);
    glGenBuffers(1, &m->vbo); glGenBuffers(1, &m->ibo);
    glBindBuffer(GL_ARRAY_BUFFER, m->vbo); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ibo);
    glEnableVertexAttribArray(0); glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, uv)));
    HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    struct ComScope { HRESULT hr; ~ComScope() { if (SUCCEEDED(hr)) CoUninitialize(); } } comScope{com};
    if (FAILED(com) && com != RPC_E_CHANGED_MODE) checked(com);
    ComPtr<IWICImagingFactory> factory;
    checked(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)));
    for (auto const& image : m->model.images) {
        ComPtr<IWICStream> stream; checked(factory->CreateStream(&stream));
        checked(stream->InitializeFromMemory(const_cast<BYTE*>(image.png.data()), static_cast<DWORD>(image.png.size())));
        ComPtr<IWICBitmapDecoder> decoder;
        checked(factory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, &decoder));
        ComPtr<IWICBitmapFrameDecode> frame; checked(decoder->GetFrame(0, &frame));
        ComPtr<IWICFormatConverter> converter; checked(factory->CreateFormatConverter(&converter));
        checked(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom));
        UINT w, h; checked(frame->GetSize(&w, &h));
        if (!w || !h || w > 8192 || h > 8192) throw std::runtime_error("Invalid Mita texture dimensions");
        std::vector<BYTE> pixels(static_cast<size_t>(w) * h * 4);
        checked(converter->CopyPixels(nullptr, w * 4, static_cast<UINT>(pixels.size()), pixels.data()));
        GLuint texture; glGenTextures(1, &texture); m->images.push_back(texture);
        glBindTexture(GL_TEXTURE_2D, texture); glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glGenFramebuffers(1, &m->fbo); glGenFramebuffers(1, &m->resolve);
    glGenRenderbuffers(1, &m->color); glGenRenderbuffers(1, &m->depth); glGenTextures(1, &m->output);
    GLint maxSamples; glGetIntegerv(GL_MAX_SAMPLES, &maxSamples); m->samples = std::clamp(maxSamples, 1, 4);
}
Renderer::~Renderer() = default;
void Renderer::abandonContext() {
    m->program = m->vao = m->vbo = m->ibo = 0;
    m->fbo = m->resolve = m->color = m->depth = m->output = 0;
    m->images.clear();
}
unsigned Renderer::texture() const { return m->output; }
void Renderer::render(int width, int height, double seconds, float exitWeight, glm::vec2 gaze, glm::vec2 headGaze, float swatTime) {
    State state;
    width = std::clamp(width, 64, 1920); height = std::clamp(height, 64, 2160);
    if (width != m->width || height != m->height) {
        glBindTexture(GL_TEXTURE_2D, m->output);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_FRAMEBUFFER, m->resolve);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m->output, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) throw std::runtime_error("Incomplete Mita resolve FBO");
        glBindFramebuffer(GL_FRAMEBUFFER, m->fbo);
        glBindRenderbuffer(GL_RENDERBUFFER, m->color);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, m->samples, GL_RGBA8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m->color);
        glBindRenderbuffer(GL_RENDERBUFFER, m->depth);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, m->samples, GL_DEPTH_COMPONENT24, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m->depth);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) throw std::runtime_error("Incomplete Mita render FBO");
        m->width = width; m->height = height;
    }
    m->model.pose(seconds, exitWeight, gaze, headGaze, swatTime);
    glBindFramebuffer(GL_FRAMEBUFFER, m->fbo); glViewport(0, 0, width, height);
    for (auto cap : State::caps) glDisable(cap);
    glEnable(GL_DEPTH_TEST); glEnable(GL_MULTISAMPLE);
    glDepthFunc(GL_LEQUAL); glDepthMask(GL_TRUE); glDepthRange(0, 1);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glClearColor(0, 0, 0, 0); glClearDepth(1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Blender close-up scene: camera (0,-4,1.56), height 0.48 metres; glTF is Y-up.
    float halfHeight = 0.24f, halfWidth = halfHeight * width / height;
    float centerOffset = halfWidth * (2 * CanvasAnchorX - 1);
    auto camera = glm::ortho(-halfWidth - centerOffset, halfWidth - centerOffset, -halfHeight, halfHeight, 0.1f, 10.f) *
        glm::lookAt(glm::vec3(0, 1.56f, 4), glm::vec3(0, 1.56f, 0), glm::vec3(0, 1, 0));
    glUseProgram(m->program); glUniformMatrix4fv(m->camera, 1, GL_FALSE, glm::value_ptr(camera));
    glBindVertexArray(m->vao); glBindBuffer(GL_ARRAY_BUFFER, m->vbo); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ibo);
    for (auto const& mesh : m->model.meshes) {
        glBindTexture(GL_TEXTURE_2D, m->images.at(mesh.image));
        glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data(), GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned), mesh.indices.data(), GL_STREAM_DRAW);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indices.size()), GL_UNSIGNED_INT, nullptr);
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m->fbo); glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m->resolve);
    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}
bool Renderer::hit(float u, float v) const {
    if (u < 0 || v < 0 || u >= 1 || v >= 1 || !m->width) return false;
    State state;
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m->resolve); glPixelStorei(GL_PACK_ALIGNMENT, 1);
    unsigned char pixel[4]{};
    glReadPixels(static_cast<int>(u * m->width), static_cast<int>(v * m->height), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    return pixel[3] >= 128;
}
}
#else
#include "Renderer.hpp"
#include <stdexcept>

namespace mita3d {
struct Renderer::Data {};
Renderer::Renderer(std::filesystem::path const&) {
    throw std::runtime_error("3D Mita isn't on this platform yet (desktop OpenGL 3.0 only)");
}
Renderer::~Renderer() = default;
void Renderer::abandonContext() {}
unsigned Renderer::texture() const { return 0; }
void Renderer::render(int, int, double, float, glm::vec2, glm::vec2, float) {}
bool Renderer::hit(float, float) const { return false; }
}
#endif
