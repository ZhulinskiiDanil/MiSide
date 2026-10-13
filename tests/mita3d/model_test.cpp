#include "Model.hpp"
#include "../../src/ui/GreetingPlan.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    try {
        mita3d::Model model(argv[1]);
        if (model.meshes.size() != 8 || model.images.size() != 4) throw std::runtime_error("Unexpected model structure");
        auto smile = model.meshes[2].vertices;
        model.pose(0, 1);
        size_t changed = 0;
        for (size_t i = 0; i < smile.size(); ++i)
            if (glm::length(smile[i].position - model.meshes[2].vertices[i].position) > 1e-7f) ++changed;
        if (changed < 50) throw std::runtime_error("Expressions do not change face geometry");
        model.pose(0, 0);
        for (size_t i = 0; i < smile.size(); ++i)
            if (glm::length(smile[i].position - model.meshes[2].vertices[i].position) > 1e-8f)
                throw std::runtime_error("Expression switch accumulates deformation");
        model.pose(0, 0);
        auto neutral = model.meshes;
        model.pose(1, 0); // Both timestamps are outside the procedural blink.
        for (size_t j = 0; j < neutral.size(); ++j)
            for (size_t v = 0; v < neutral[j].vertices.size(); ++v)
                if (glm::length(neutral[j].vertices[v].position - model.meshes[j].vertices[v].position) > 1e-8f)
                    throw std::runtime_error("Swat plays without a click trigger");
        model.pose(0, 0, {1, 1});
        size_t eyeMotion = 0;
        for (size_t j = 0; j < neutral.size(); ++j)
            for (size_t v = 0; v < neutral[j].vertices.size(); ++v)
                if (glm::length(neutral[j].vertices[v].position - model.meshes[j].vertices[v].position) > 1e-6f) ++eyeMotion;
        if (eyeMotion < 20) throw std::runtime_error("Eye bones do not affect geometry");
        model.pose(13.2, 0.5f, {1, -1});
        auto direct = model.meshes[2].vertices;
        for (int i = 0; i <= 792; ++i) model.pose(i / 60.0, 0.5f, {1, -1});
        for (size_t i = 0; i < direct.size(); ++i)
            if (glm::length(direct[i].position - model.meshes[2].vertices[i].position) > 1e-8f)
                throw std::runtime_error("Animation depends on frame count");
        for (double t : {0., 4.39, 5.7, 8.123, 12.99, 13., 10000.}) {
            model.pose(t, 0.5f, {});
            for (auto const& mesh : model.meshes) for (auto const& v : mesh.vertices)
                if (!std::isfinite(v.position.x) || !std::isfinite(v.position.y) || !std::isfinite(v.position.z) || glm::length(v.position) > 5.f)
                    throw std::runtime_error("Invalid skinned position");
        }
        for (double idleTime : {0., 0.83, 4.39, 12.99}) {
            auto pose = [&](float swatTime) { model.pose(idleTime, 0.3f, {-0.7f, 0.5f}, {-0.4f, 0.2f}, swatTime); };
            pose(-1);
            auto idle = model.meshes;
            auto displacement = [&] {
                float maximum = 0;
                for (size_t mesh = 0; mesh < idle.size(); ++mesh)
                    for (size_t v = 0; v < idle[mesh].vertices.size(); ++v)
                        maximum = std::max(maximum, glm::length(idle[mesh].vertices[v].position - model.meshes[mesh].vertices[v].position));
                return maximum;
            };
            for (float endpoint : {0.f, mita3d::SwatDuration}) {
                pose(endpoint);
                auto endpointDisplacement = displacement();
                if (endpointDisplacement > 1e-6f)
                    throw std::runtime_error("Swat endpoint displacement " + std::to_string(endpointDisplacement));
            }
            for (float nearEndpoint : {0.001f, mita3d::SwatDuration - 0.001f}) {
                pose(nearEndpoint);
                if (displacement() > 1e-6f) throw std::runtime_error("Swat jumps at an animation boundary");
            }
            size_t movedBody = 0;
            float largestMotion = 0;
            for (int frame = 1; frame < 28; ++frame) {
                pose(mita3d::SwatDuration * frame / 28);
                for (size_t mesh = 0; mesh < idle.size(); ++mesh) {
                    for (size_t v = 0; v < idle[mesh].vertices.size(); ++v) {
                        auto before = idle[mesh].vertices[v].position;
                        auto after = model.meshes[mesh].vertices[v].position;
                        if (!std::isfinite(after.x) || !std::isfinite(after.y) || !std::isfinite(after.z) || glm::length(after) > 5.f)
                            throw std::runtime_error("Swat creates invalid skinned geometry");
                        float motion = glm::length(after - before);
                        if (mesh != 0 && motion > 1e-5f) ++movedBody;
                        largestMotion = std::max(largestMotion, motion);
                    }
                }
            }
            if (movedBody < 50 || largestMotion < 0.05f) throw std::runtime_error("Swat does not raise the hand");
            pose(0);
            auto previous = model.meshes;
            for (int frame = 1; frame <= 840; ++frame) {
                pose(mita3d::SwatDuration * frame / 840);
                for (size_t mesh = 0; mesh < previous.size(); ++mesh)
                    for (size_t v = 0; v < previous[mesh].vertices.size(); ++v)
                        if (glm::length(previous[mesh].vertices[v].position - model.meshes[mesh].vertices[v].position) > 0.016f)
                            throw std::runtime_error("Swat rotation jump at " + std::to_string(idleTime) + ":" + std::to_string(frame) + " mesh " + std::to_string(mesh) + " vertex " + std::to_string(v) + " distance " + std::to_string(glm::length(previous[mesh].vertices[v].position - model.meshes[mesh].vertices[v].position)));
                previous = model.meshes;
            }
            pose(-1);
            if (displacement() > 1e-6f) throw std::runtime_error("Swat leaves accumulated deformation after returning to idle");
        }
        // The looped idle: off unless asked for, moves her, stays finite, and the swat still starts and ends on it
        if (!model.hasIdle() || std::abs(model.idleDuration() - 4.f) > 0.001f) throw std::runtime_error("Missing looped idle");
        model.pose(1, 0);
        auto still = model.meshes;
        auto distance = [&](std::vector<mita3d::Mesh> const& from) {
            float maximum = 0;
            for (size_t mesh = 0; mesh < from.size(); ++mesh)
                for (size_t v = 0; v < from[mesh].vertices.size(); ++v)
                    maximum = std::max(maximum, glm::length(from[mesh].vertices[v].position - model.meshes[mesh].vertices[v].position));
            return maximum;
        };
        float idleMotion = 0;
        for (float idleTime : {0.5f, 1.f, 2.f, 3.f, 3.9f, 41.f}) {
            model.pose(1, 0, {}, {}, -1.f, 0.f, idleTime);
            for (auto const& mesh : model.meshes) for (auto const& v : mesh.vertices)
                if (!std::isfinite(v.position.x) || !std::isfinite(v.position.y) || !std::isfinite(v.position.z) || glm::length(v.position) > 5.f)
                    throw std::runtime_error("Idle creates invalid skinned geometry");
            idleMotion = std::max(idleMotion, distance(still));
            auto idle = model.meshes;
            for (float endpoint : {0.f, mita3d::SwatDuration}) {
                model.pose(1, 0, {}, {}, endpoint, 0.f, idleTime);
                if (distance(idle) > 1e-6f) throw std::runtime_error("Swat does not start and end on the idle");
            }
        }
        if (idleMotion < 0.01f) throw std::runtime_error("Idle does not move her");
        model.pose(1, 0, {}, {}, -1.f, 0.f, 0.f);
        auto loopStart = model.meshes;
        model.pose(1, 0, {}, {}, -1.f, 0.f, model.idleDuration() - 0.001f);
        std::cout << "Idle: " << model.idleDuration() << " s, largest motion " << idleMotion << " m, loop seam " << distance(loopStart) << " m\n";
        // The greeting holds its end rather than wrapping, and hands every vertex back to the menu.
        auto duration = model.idleDuration();
        model.pose(1, 0, {}, {}, -1, 0, duration, 1, false);
        auto last = model.meshes;
        model.pose(1, 0, {}, {}, -1, 0, duration + 1, 1, false);
        if (distance(last) > 1e-7f) throw std::runtime_error("Greeting wraps past its last frame");
        model.pose(1, 0, {}, {}, -1, 0, duration, 0, false);
        if (distance(still) > 1e-6f) throw std::runtime_error("Greeting does not return to the menu pose");
        auto state = greeting::at(0, duration);
        if (state.model != 0 || state.veil != 1 || state.showInterface) throw std::runtime_error("Greeting starts exposed");
        auto previous = last;
        for (float clock = duration; clock <= greeting::end(duration) + 0.01f; clock += 0.001f) {
            state = greeting::at(clock, duration);
            model.pose(1, 0, {}, {}, -1, 0, state.animation, state.weight, false);
            // Skip the initial sample: blending begins slightly before the clip ends.
            if (clock > duration + 0.001f && distance(previous) > 0.003f)
                throw std::runtime_error("Greeting jumps during the return to the menu");
            previous = model.meshes;
        }
        state = greeting::at(greeting::end(duration), duration);
        if (!state.done || !state.showInterface || state.zoom != 0 || state.aura != 0 || state.weight != 0 || state.model != 1 || state.veil != 0)
            throw std::runtime_error("Greeting leaves presentation offsets behind");
        std::cout << "PASS: greeting last-frame hold, continuous blend and exact menu endpoint\n";
        std::cout << "PASS: expressions, reversible pose, frame-independent animation, finite rig, continuous right-hand swat, looped idle; changed face vertices " << changed << '\n';
    } catch (std::exception const& e) { std::cerr << e.what() << '\n'; return 1; }
}
