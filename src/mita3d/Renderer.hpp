#pragma once
#include "Model.hpp"
#include <memory>

namespace mita3d {
inline constexpr float CanvasAspect = 4.f / 3.f;
inline constexpr float CanvasAnchorX = 2.f / 3.f;
inline constexpr float OriginalCanvasFraction = 2.f / 3.f;
class Renderer {
public:
    explicit Renderer(std::filesystem::path const& model);
    ~Renderer();
    Renderer(Renderer const&) = delete;
    Renderer& operator=(Renderer const&) = delete;
    void render(int width, int height, double seconds, float exitWeight, glm::vec2 gaze, glm::vec2 headGaze = {}, float swatTime = -1.f);
    unsigned texture() const;
    bool hit(float u, float v) const;
    void abandonContext();
private:
    struct Data;
    std::unique_ptr<Data> m;
};
}
