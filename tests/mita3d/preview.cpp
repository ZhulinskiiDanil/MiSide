#define NOMINMAX
#include <Windows.h>
#include <GL/glew.h>
#include "Renderer.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>

static constexpr int PreviewWidth = static_cast<int>(1080 * mita3d::CanvasAspect);

int main(int argc, char** argv) {
    if (argc < 3) return 2;
    WNDCLASSW cls{}; cls.lpfnWndProc = DefWindowProcW; cls.hInstance = GetModuleHandleW(nullptr); cls.lpszClassName = L"Mita3DPreview";
    RegisterClassW(&cls);
    auto window = CreateWindowW(cls.lpszClassName, L"Mita render test", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr, cls.hInstance, nullptr);
    auto dc = GetDC(window);
    PIXELFORMATDESCRIPTOR pfd{}; pfd.nSize = sizeof(pfd); pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER; pfd.iPixelType = PFD_TYPE_RGBA; pfd.cColorBits = 32; pfd.cDepthBits = 24;
    SetPixelFormat(dc, ChoosePixelFormat(dc, &pfd), &pfd);
    auto context = wglCreateContext(dc); wglMakeCurrent(dc, context);
    int result = 0;
    try {
        if (glewInit() != GLEW_OK) throw std::runtime_error("GLEW initialization failed");
        while (glGetError() != GL_NO_ERROR) {}
        std::cout << "OpenGL " << glGetString(GL_VERSION) << " / " << glGetString(GL_RENDERER) << '\n';
        {
            mita3d::Model model(argv[1]);
            glm::vec3 lo(1e9f), hi(-1e9f);
            for (auto const& mesh : model.meshes) for (auto const& vertex : mesh.vertices) {
                lo = glm::min(lo, vertex.position); hi = glm::max(hi, vertex.position);
            }
            std::cout << "Bounds " << lo.x << ',' << lo.y << ',' << lo.z << " - " << hi.x << ',' << hi.y << ',' << hi.z << '\n';
        }
        auto start = std::chrono::steady_clock::now();
        mita3d::Renderer renderer(argv[1]);
        std::cout << "Load ms " << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() << '\n';
        // Deliberately non-default state; it must survive both render and hit testing.
        glViewport(3, 7, 39, 41); glEnable(GL_SCISSOR_TEST); glEnable(GL_BLEND);
        glDepthMask(GL_FALSE); glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_FALSE);
        glActiveTexture(GL_TEXTURE3);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 23); glPixelStorei(GL_PACK_ROW_LENGTH, 19);
        float expression = argc > 3 ? std::stof(argv[3]) : 0;
        double seconds = argc > 4 ? std::stod(argv[4]) : 0;
        glm::vec2 gaze = argc > 6 ? glm::vec2(std::stof(argv[5]), std::stof(argv[6])) : glm::vec2(0);
        float swatTime = argc > 7 ? std::stof(argv[7]) : -1.f;
        renderer.render(PreviewWidth, 1080, seconds, expression, gaze, gaze, swatTime, expression);
        GLint viewport[4], active; GLboolean depth, color[4];
        glGetIntegerv(GL_VIEWPORT, viewport); glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depth); glGetBooleanv(GL_COLOR_WRITEMASK, color);
        if (viewport[0] != 3 || viewport[3] != 41 || active != GL_TEXTURE3 || depth || color[1] || color[3] ||
            !glIsEnabled(GL_SCISSOR_TEST) || !glIsEnabled(GL_BLEND)) throw std::runtime_error("GL state leaked");
        bool center = renderer.hit(mita3d::CanvasAnchorX, 0.5f), corner = renderer.hit(0, 0.99f);
        if (!center || corner || renderer.hit(-1, 0.5f) || renderer.hit(1, 0.5f)) throw std::runtime_error("Silhouette hit test failed");
        GLint row; glGetIntegerv(GL_PACK_ROW_LENGTH, &row);
        if (row != 19) throw std::runtime_error("Pixel storage state leaked");
        auto error = glGetError(); if (error != GL_NO_ERROR) throw std::runtime_error("GL error " + std::to_string(error));
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, renderer.texture()); glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_PACK_ROW_LENGTH, 0);
        std::vector<unsigned char> pixels(PreviewWidth * 1080 * 4);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        std::ofstream out(argv[2], std::ios::binary);
        out.write(reinterpret_cast<char*>(pixels.data()), pixels.size());
        int frames = argc > 8 ? std::stoi(argv[8]) : 1;
        if (frames < 1 || frames > 300) throw std::runtime_error("Invalid preview frame count");
        for (int frame = 1; frame < frames; ++frame) {
            float dt = frame / 30.f;
            renderer.render(PreviewWidth, 1080, seconds + dt, expression, gaze, gaze, swatTime + dt, expression);
            glBindTexture(GL_TEXTURE_2D, renderer.texture());
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            out.write(reinterpret_cast<char*>(pixels.data()), pixels.size());
        }
        out.close();
        glFinish(); start = std::chrono::steady_clock::now();
        for (int i = 0; i < 120; ++i) renderer.render(PreviewWidth, 1080, i / 30.0, expression, {}, {}, -1.f, expression);
        glFinish();
        std::cout << "Render average ms " << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() / 120 << '\n';
        renderer.render(720, 540, 4, expression, {});
        renderer.render(PreviewWidth, 1080, 5, expression, {});
        GLuint releasedTexture = 0;
        {
            mita3d::Renderer reopened(argv[1]);
            reopened.render(720, 540, 0, 0, {});
            releasedTexture = reopened.texture();
        }
        if (glIsTexture(releasedTexture) || !glIsTexture(renderer.texture())) throw std::runtime_error("Texture ownership failed");
        if (glGetError() != GL_NO_ERROR) throw std::runtime_error("Resize GL error");
        std::cout << "PASS: state restoration, hit test, render, resize and resource release\n";
    } catch (std::exception const& e) { std::cerr << e.what() << '\n'; result = 1; }
    wglMakeCurrent(nullptr, nullptr); wglDeleteContext(context); ReleaseDC(window, dc); DestroyWindow(window);
    return result;
}
