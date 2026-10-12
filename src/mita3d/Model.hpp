#pragma once

#include <glm/glm.hpp>
#include <filesystem>
#include <memory>
#include <vector>

namespace mita3d {
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
    void pose(double seconds, float exitWeight, glm::vec2 gaze = {});
    std::vector<Mesh> meshes;
    std::vector<Image> images;
private:
    struct Data;
    std::unique_ptr<Data> m;
};
}
