#pragma once

#include <glm/glm.hpp>
#include <filesystem>
#include <memory>
#include <vector>

namespace mita3d {
inline constexpr float SwatDuration = 2.8f;
// The optional looped idle: played under everything else while asked for (pose's idleTime), skipped when missing
inline constexpr char const* IdleAnimation = "Mita_Greeting";
inline constexpr float ModelYaw = -24.f * 3.14159265f / 180.f;
inline constexpr float GazeYawRange = 0.43f;
// Displeased, the irises turn this much more to the viewer than her face (they rest a little to her right)
inline constexpr float ExitEyesYaw = 0.12f;
// The camera looks down at her from a little above; her head and eyes are raised as much, so she looks as high as
// she would to a level camera
inline constexpr float CameraPitch = 7.f * 3.14159265f / 180.f;
// How far her head follows the gaze up and down, the eyes the rest
inline constexpr float HeadPitchRange = 0.14f;
// Facing the viewer, her head tilts this much to the viewer's right
inline constexpr float ViewerHeadTilt = 6.f * 3.14159265f / 180.f;
// Facing the viewer, her head turns this much to the viewer's left and rises this much (above the camera's height:
// she looks down at the viewer), her eyes still on the camera
inline constexpr float ViewerHeadAside = 5.f * 3.14159265f / 180.f;
inline constexpr float ViewerChinUp = 11.f * 3.14159265f / 180.f;
// Facing the viewer, her eyes narrow this much (of a blink): a little upset
inline constexpr float ViewerSquint = 0.12f;
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
    // idleTime: seconds into the looped idle (IdleAnimation), negative for none
    void pose(double seconds, float exitWeight, glm::vec2 gaze = {}, glm::vec2 headGaze = {}, float swatTime = -1.f,
              float facing = 0.f, float idleTime = -1.f, float idleWeight = 1.f, bool idleLoop = true);
    // The model has the idle (and how long one loop is)
    bool hasIdle() const;
    float idleDuration() const;
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
