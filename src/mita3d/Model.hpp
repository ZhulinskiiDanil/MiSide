#pragma once

#include <glm/glm.hpp>
#include <filesystem>
#include <memory>
#include <vector>

namespace mita3d {
inline constexpr float SwatDuration = 2.8f;
inline constexpr float ModelYaw = -20.f * 3.14159265f / 180.f;
inline constexpr float GazeYawRange = 0.43f;
struct Vertex { glm::vec3 position; glm::vec2 uv; };
struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned> indices;
    unsigned image = 0;
};
struct Image { std::vector<unsigned char> png; };

// CPU pose evaluation is shared by the game and the standalone renderer tests.
class Model {
public:
    explicit Model(std::filesystem::path const& path);
    ~Model();
    Model(Model const&) = delete;
    Model& operator=(Model const&) = delete;
    void pose(double seconds, float exitWeight, glm::vec2 gaze = {}, glm::vec2 headGaze = {}, float swatTime = -1.f);
    std::vector<Mesh> meshes;
    std::vector<Image> images;
#ifdef MITA3D_TOOLING
    // Export the exact runtime rig for inspection in Blender.
    void exportPose(std::filesystem::path const& path) const;
#endif
private:
    struct Data;
    std::unique_ptr<Data> m;
};
}
