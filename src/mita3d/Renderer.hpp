#pragma once
#include "Model.hpp"
#include <memory>

namespace mita3d {
// The canvas: 1.6 heights wide, her middle 8/9 of a height from its left edge; the room to its right keeps her
// ponytail in it
inline constexpr float CanvasAspect = 1.6f;
inline constexpr float CanvasAnchorX = 5.f / 9.f;
// MiSide's own frame (8/9 of a height wide, centred on her middle) as a part of the canvas' width
inline constexpr float OriginalCanvasFraction = 5.f / 9.f;
class Renderer {
public:
    explicit Renderer(std::filesystem::path const& model);
    ~Renderer();
    Renderer(Renderer const&) = delete;
    Renderer& operator=(Renderer const&) = delete;
    void render(int width, int height, double seconds, float exitWeight, glm::vec2 gaze, glm::vec2 headGaze = {}, float swatTime = -1.f,
                float facing = 0.f, float idleTime = -1.f, float idleWeight = 1.f, bool idleLoop = true, float zoom = 0.f);
    bool hasIdle() const;
    float idleDuration() const;
    unsigned texture() const;
    bool hit(float u, float v) const;
    void abandonContext();
private:
    struct Data;
    std::unique_ptr<Data> m;
};
}
