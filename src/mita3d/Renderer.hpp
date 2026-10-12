#pragma once
#include "Model.hpp"
#include <memory>

namespace mita3d {
class Renderer {
public:
    explicit Renderer(std::filesystem::path const& model);
    ~Renderer();
    Renderer(Renderer const&) = delete;
    Renderer& operator=(Renderer const&) = delete;
    void render(int width, int height, double seconds, float exitWeight, glm::vec2 gaze, glm::vec2 headGaze = {});
    unsigned texture() const;
    bool hit(float u, float v) const;
    void abandonContext();
private:
    struct Data;
    std::unique_ptr<Data> m;
};
}
