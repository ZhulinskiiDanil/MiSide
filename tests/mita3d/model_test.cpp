#include "Model.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    try {
        mita3d::Model model(argv[1]);
        if (model.meshes.size() != 8 || model.images.size() != 4) throw std::runtime_error("Unexpected model structure");
        auto smile = model.meshes.front().vertices;
        model.pose(0, 1);
        size_t changed = 0;
        for (size_t i = 0; i < smile.size(); ++i)
            if (glm::length(smile[i].position - model.meshes.front().vertices[i].position) > 1e-7f) ++changed;
        if (changed < 50) throw std::runtime_error("Expressions do not change face geometry");
        model.pose(0, 0);
        for (size_t i = 0; i < smile.size(); ++i)
            if (glm::length(smile[i].position - model.meshes.front().vertices[i].position) > 1e-8f)
                throw std::runtime_error("Expression switch accumulates deformation");
        model.pose(13.2, 0.5f, {1, -1});
        auto direct = model.meshes.front().vertices;
        for (int i = 0; i <= 792; ++i) model.pose(i / 60.0, 0.5f, {1, -1});
        for (size_t i = 0; i < direct.size(); ++i)
            if (glm::length(direct[i].position - model.meshes.front().vertices[i].position) > 1e-8f)
                throw std::runtime_error("Animation depends on frame count");
        for (double t : {0., 4.39, 5.7, 8.123, 12.99, 13., 10000.}) {
            model.pose(t, 0.5f, {});
            for (auto const& mesh : model.meshes) for (auto const& v : mesh.vertices)
                if (!std::isfinite(v.position.x) || !std::isfinite(v.position.y) || !std::isfinite(v.position.z) || glm::length(v.position) > 0.05f)
                    throw std::runtime_error("Invalid skinned position");
        }
        std::cout << "PASS: expressions, reversible pose, frame-independent animation, finite rig; changed face vertices " << changed << '\n';
    } catch (std::exception const& e) { std::cerr << e.what() << '\n'; return 1; }
}
