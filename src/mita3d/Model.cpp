#include "Model.hpp"
#include "EyeVertices.hpp"
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>

namespace mita3d {
namespace {
void require(bool ok, char const* message) { if (!ok) throw std::runtime_error(message); }
glm::vec4 read(cgltf_accessor const* a, size_t index) {
    glm::vec4 v(0);
    require(a && cgltf_accessor_read_float(a, index, &v.x, 4), "Invalid model accessor");
    return v;
}
cgltf_accessor* attribute(cgltf_attribute* a, size_t count, cgltf_attribute_type type, int index = 0) {
    for (size_t i = 0; i < count; ++i) if (a[i].type == type && a[i].index == index) return a[i].data;
    return nullptr;
}
glm::quat quaternion(glm::vec4 v) { return glm::quat(v.w, v.x, v.y, v.z); }
float ease(float value) {
    float u = glm::clamp(value, 0.f, 1.f);
    return u * u * u * (u * (u * 6 - 15) + 10);
}
glm::quat aimBone(glm::mat3 const& basis, glm::vec3 direction) {
    auto from = glm::normalize(basis[1]);
    auto to = glm::normalize(direction);
    float dot = glm::dot(from, to);
    auto turn = dot < -0.9999f ? glm::angleAxis(3.14159265f, glm::normalize(basis[0])) :
        glm::normalize(glm::quat(1 + dot, glm::cross(from, to)));
    return glm::normalize(turn * glm::quat_cast(basis));
}
void orientBone(glm::mat4& world, glm::quat rotation) {
    auto basis = glm::mat3_cast(rotation);
    for (int axis = 0; axis < 3; ++axis)
        world[axis] = glm::vec4(basis[axis] * glm::length(glm::vec3(world[axis])), 0);
}
}
struct Model::Data {
    std::vector<char> bytes;
    std::unique_ptr<cgltf_data, decltype(&cgltf_free)> gltf{nullptr, cgltf_free};
    struct Source {
        cgltf_node* node;
        bool face = false;
        std::vector<glm::vec3> positions;
        std::array<std::vector<glm::vec3>, 2> morphs;
        std::vector<glm::uvec4> joints;
        std::vector<glm::vec4> weights;
        std::vector<glm::mat4> inverseBind;
    };
    struct Channel {
        size_t node;
        cgltf_animation_path_type path;
        std::vector<float> times;
        std::vector<glm::vec4> values;
    };
    std::vector<Source> sources;
    std::vector<Channel> channels;
    std::vector<glm::vec3> translations, scales;
    std::vector<glm::quat> rotations;
    std::vector<glm::mat4> world;
    std::vector<size_t> order;
    glm::mat3 headRestOrientation{1};
    std::vector<glm::mat3> restOrientation;
    std::array<int, 3> leftArm{-1, -1, -1}, rightArm{-1, -1, -1};
    std::vector<size_t> rightFingers;
    std::array<int, 3> leftTwist{-1, -1, -1}, rightTwist{-1, -1, -1};
    float duration = 0;
    int head = -1, torso = -1;
};
Model::~Model() = default;
Model::Model(std::filesystem::path const& path) : m(std::make_unique<Data>()) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    require(bool(file), "Cannot open mita.glb");
    auto size = file.tellg();
    require(size > 0 && size < 128 * 1024 * 1024, "Invalid model size");
    m->bytes.resize(static_cast<size_t>(size));
    file.seekg(0); file.read(m->bytes.data(), size);
    require(bool(file), "Cannot read mita.glb");
    cgltf_options options{};
    cgltf_data* raw = nullptr;
    require(cgltf_parse(&options, m->bytes.data(), m->bytes.size(), &raw) == cgltf_result_success, "Invalid GLB");
    m->gltf.reset(raw);
    require(raw->file_type == cgltf_file_type_glb && raw->buffers_count == 1 && !raw->buffers[0].uri,
            "Mita requires a self-contained GLB");
    require(cgltf_load_buffers(&options, raw, nullptr) == cgltf_result_success &&
            cgltf_validate(raw) == cgltf_result_success, "Invalid GLB buffers");
    for (size_t i = 0; i < raw->images_count; ++i) {
        auto const& image = raw->images[i];
        require(image.buffer_view && image.mime_type && std::string(image.mime_type) == "image/png", "Expected embedded PNG");
        auto p = cgltf_buffer_view_data(image.buffer_view);
        require(p != nullptr, "Missing embedded image");
        images.push_back({{p, p + image.buffer_view->size}});
    }
    auto n = raw->nodes_count;
    m->translations.resize(n); m->scales.resize(n); m->rotations.resize(n);
    m->world.resize(n); m->restOrientation.resize(n);
    auto visit = [&](auto&& self, cgltf_node* node) -> void {
        m->order.push_back(node - raw->nodes);
        for (size_t j = 0; j < node->children_count; ++j) self(self, node->children[j]);
    };
    for (size_t i = 0; i < n; ++i) {
        auto& node = raw->nodes[i];
        if (!node.parent) visit(visit, &node);
        std::string name = node.name ? node.name : "";
        if (name == "Spine_082_128") m->torso = static_cast<int>(i);
        if (name == "Head_0125_182") m->head = static_cast<int>(i);
        constexpr char const* leftNames[] = {"Left arm_085_131", "Left elbow_086_132", "Left wrist_087_133"};
        constexpr char const* rightNames[] = {"Right arm_0159_224", "Right elbow_0160_225", "Right wrist_0161_226"};
        for (int k = 0; k < 3; ++k) {
            if (name == leftNames[k]) m->leftArm[k] = static_cast<int>(i);
            if (name == rightNames[k]) m->rightArm[k] = static_cast<int>(i);
        }
        constexpr char const* leftTwists[] = {"TwistForearm.L_0110_163", "TwistForearm1.L_0113_166", "TwistForearm2.L_0115_169"};
        constexpr char const* rightTwists[] = {"TwistForearm.R_0184_256", "TwistForearm1.R_0186_259", "TwistForearm2.R_0188_262"};
        for (int k = 0; k < 3; ++k) {
            if (name == leftTwists[k]) m->leftTwist[k] = static_cast<int>(i);
            if (name == rightTwists[k]) m->rightTwist[k] = static_cast<int>(i);
        }
        if (name.find("Finger") != std::string::npos && name.find("_R_") != std::string::npos &&
            name.find("end") == std::string::npos) m->rightFingers.push_back(i);
        float restMatrix[16];
        cgltf_node_transform_world(&node, restMatrix);
        auto rest = glm::make_mat4(restMatrix);
        for (int axis = 0; axis < 3; ++axis)
            m->restOrientation[i][axis] = glm::normalize(glm::vec3(rest[axis]));
        if (!node.mesh) continue;
        require(node.skin != nullptr, "Expected skinned Mita mesh");
        for (size_t j = 0; j < node.mesh->primitives_count; ++j) {
            auto& primitive = node.mesh->primitives[j];
            require(primitive.type == cgltf_primitive_type_triangles && primitive.indices, "Expected indexed triangles");
            auto position = attribute(primitive.attributes, primitive.attributes_count, cgltf_attribute_type_position);
            auto uv = attribute(primitive.attributes, primitive.attributes_count, cgltf_attribute_type_texcoord);
            auto joints = attribute(primitive.attributes, primitive.attributes_count, cgltf_attribute_type_joints);
            auto weights = attribute(primitive.attributes, primitive.attributes_count, cgltf_attribute_type_weights);
            require(position && uv && joints && weights, "Incomplete Mita vertex attributes");
            auto material = primitive.material;
            require(material && material->emissive_texture.texture && material->emissive_texture.texture->image,
                    "Expected authored emissive Mita material");
            Mesh mesh;
            mesh.image = static_cast<unsigned>(material->emissive_texture.texture->image - raw->images);
            Data::Source source; source.node = &node;
            source.face = primitive.targets_count > 0;
            if (source.face) require(position->count == 2451, "Unexpected face topology for iris tracking");
            for (size_t v = 0; v < position->count; ++v) {
                source.positions.emplace_back(read(position, v));
                source.joints.emplace_back(read(joints, v));
                auto w = read(weights, v);
                float sum = w.x + w.y + w.z + w.w;
                source.weights.push_back(sum > 0 ? w / sum : glm::vec4(0));
                for (int k = 0; k < 4; ++k)
                    require(source.joints.back()[k] < node.skin->joints_count, "Skin joint out of range");
                mesh.vertices.push_back({source.positions.back(), glm::vec2(read(uv, v))});
            }
            // Blink and menu smile. Exit uses the unmodified basis pose.
            constexpr unsigned targets[] = {1, 14};
            for (int k = 0; k < 2; ++k) {
                source.morphs[k].resize(position->count, glm::vec3(0));
                if (!primitive.targets_count) continue;
                require(primitive.targets_count > targets[k], "Missing expression targets");
                auto& target = primitive.targets[targets[k]];
                auto delta = attribute(target.attributes, target.attributes_count, cgltf_attribute_type_position);
                if (delta) for (size_t v = 0; v < position->count; ++v) source.morphs[k][v] = glm::vec3(read(delta, v));
            }
            for (size_t k = 0; k < primitive.indices->count; ++k) {
                auto index = cgltf_accessor_read_index(primitive.indices, k);
                require(index < mesh.vertices.size(), "Mesh index out of range");
                mesh.indices.push_back(static_cast<unsigned>(index));
            }
            for (size_t k = 0; k < node.skin->joints_count; ++k) {
                glm::mat4 matrix(1);
                if (node.skin->inverse_bind_matrices)
                    require(cgltf_accessor_read_float(node.skin->inverse_bind_matrices, k, glm::value_ptr(matrix), 16), "Invalid bind matrix");
                source.inverseBind.push_back(matrix);
            }
            meshes.push_back(std::move(mesh)); m->sources.push_back(std::move(source));
        }
    }
    require(m->head >= 0 && m->torso >= 0 && !meshes.empty(), "Mita rig does not match");
    for (int k = 0; k < 3; ++k)
        require(m->leftArm[k] >= 0 && m->rightArm[k] >= 0, "Missing arm rig");
    float rest[16];
    cgltf_node_transform_world(&raw->nodes[m->head], rest);
    auto restWorld = glm::make_mat4(rest);
    for (int axis = 0; axis < 3; ++axis)
        m->headRestOrientation[axis] = glm::normalize(glm::vec3(restWorld[axis]));
    require(raw->animations_count > 0, "Missing author idle animation");
    auto& animation = raw->animations[0];
    for (size_t i = 0; i < animation.channels_count; ++i) {
        auto& c = animation.channels[i];
        if (c.target_path == cgltf_animation_path_type_weights) continue; // Expressions are interactive.
        require(c.target_node && c.sampler && c.sampler->interpolation == cgltf_interpolation_type_linear,
                "Unsupported idle interpolation");
        Data::Channel channel{static_cast<size_t>(c.target_node - raw->nodes), c.target_path, {}, {}};
        for (size_t k = 0; k < c.sampler->input->count; ++k) {
            channel.times.push_back(read(c.sampler->input, k).x);
            channel.values.push_back(read(c.sampler->output, k));
        }
        require(!channel.times.empty(), "Empty idle channel");
        m->duration = std::max(m->duration, channel.times.back());
        m->channels.push_back(std::move(channel));
    }
    require(m->duration > 0, "Empty idle animation");
    pose(0, 0);
}
void Model::pose(double seconds, float exitWeight, glm::vec2 gaze, glm::vec2 headGaze, float swatTime) {
    auto raw = m->gltf.get();
    float t = static_cast<float>(std::fmod(std::max(seconds, 0.0) * 0.65, m->duration));
    for (size_t i = 0; i < raw->nodes_count; ++i) {
        m->translations[i] = glm::make_vec3(raw->nodes[i].translation);
        m->rotations[i] = glm::make_quat(raw->nodes[i].rotation);
        m->scales[i] = glm::make_vec3(raw->nodes[i].scale);
    }
    for (auto const& c : m->channels) {
        size_t b = std::upper_bound(c.times.begin(), c.times.end(), t) - c.times.begin();
        b = std::min(b, c.times.size() - 1);
        size_t a = b ? b - 1 : 0;
        float dt = c.times[b] - c.times[a];
        float u = dt > 0 ? glm::clamp((t - c.times[a]) / dt, 0.f, 1.f) : 0.f;
        if (c.path == cgltf_animation_path_type_rotation)
            m->rotations[c.node] = glm::normalize(glm::slerp(quaternion(c.values[a]), quaternion(c.values[b]), u));
        else if (c.path == cgltf_animation_path_type_translation)
            m->translations[c.node] = glm::vec3(glm::mix(c.values[a], c.values[b], u));
        else if (c.path == cgltf_animation_path_type_scale)
            m->scales[c.node] = glm::vec3(glm::mix(c.values[a], c.values[b], u));
    }
    float swatWeight = swatTime >= 0 && swatTime < SwatDuration ?
        ease(swatTime / 0.55f) * ease((SwatDuration - swatTime) / 0.7f) : 0;
    float wavePhase = glm::clamp((swatTime - 0.55f) / 1.55f, 0.f, 1.f);
    float wave = std::sin(wavePhase * 12.5663706f) * std::sin(wavePhase * 3.14159265f);
    for (auto i : m->rightFingers)
        m->rotations[i] = glm::slerp(m->rotations[i], glm::quat(1, 0, 0, 0), swatWeight * 0.85f);
    auto torsoTurn = glm::mat3_cast(glm::angleAxis(-0.40f, glm::vec3(0, 1, 0)));
    glm::vec3 waveElbow(0), waveWrist(0);
    float swayTime = static_cast<float>(std::fmod(std::max(seconds, 0.0), 13.0));
    float sway = swayTime > 9 ? std::sin((swayTime - 9) / 4 * 6.2831853f) * 0.025f : 0;
    gaze = glm::clamp(gaze, glm::vec2(-1), glm::vec2(1));
    headGaze = glm::clamp(headGaze, glm::vec2(-1), glm::vec2(1));
    for (auto i : m->order) {
        auto& node = raw->nodes[i];
        auto local = node.has_matrix ? glm::make_mat4(node.matrix) :
            glm::translate(glm::mat4(1), m->translations[i]) * glm::mat4_cast(m->rotations[i]) *
            glm::scale(glm::mat4(1), m->scales[i]);
        auto parent = node.parent ? m->world[node.parent - raw->nodes] : glm::mat4(1);
        auto world = parent * local;
        float yaw = 0, pitch = 0, roll = 0;
        if (static_cast<int>(i) == m->torso) yaw = -0.40f;
        if (static_cast<int>(i) == m->head) {
            // Aim from the neutral rig pose, not on top of the idle clip's head tilt.
            // The neck position and the rest of the body keep the author's animation.
            for (int axis = 0; axis < 3; ++axis)
                world[axis] = glm::vec4(m->headRestOrientation[axis] * glm::length(glm::vec3(world[axis])), 0);
            yaw = headGaze.x * 0.27f; pitch = -headGaze.y * 0.09f; roll = sway;
        }
        if (yaw || pitch || roll) {
            auto pivot = glm::vec3(world[3]);
            world = glm::translate(glm::mat4(1), pivot) *
                    glm::rotate(glm::mat4(1), yaw, glm::vec3(0, 1, 0)) *
                    glm::rotate(glm::mat4(1), pitch, glm::vec3(1, 0, 0)) *
                    glm::rotate(glm::mat4(1), roll, glm::vec3(0, 0, 1)) *
                    glm::translate(glm::mat4(1), -pivot) * world;
        }
        for (int side = 0; side < 2; ++side) {
            auto const& arm = side ? m->rightArm : m->leftArm;
            float sign = side ? -1.f : 1.f;
            for (int joint = 0; joint < 3; ++joint) {
                if (static_cast<int>(i) != arm[joint]) continue;
                auto restBasis = torsoTurn * m->restOrientation[i];
                glm::vec3 down = torsoTurn * glm::vec3(sign * 0.12f, -1.f, joint ? 0.12f : 0.f);
                auto idle = aimBone(restBasis, down);
                auto raised = idle;
                if (side && swatWeight > 0) {
                    if (joint == 0) {
                        auto shoulder = glm::vec3(world[3]);
                        waveWrist = glm::vec3(m->world[m->head][3]) + glm::vec3(-0.00055f + wave * 0.00065f, -0.00085f, 0.00255f);
                        float upper = glm::length(glm::vec3(world[1])) * glm::length(m->translations[arm[1]]);
                        float lower = glm::length(glm::vec3(world[1])) * glm::length(m->translations[arm[2]]);
                        auto delta = waveWrist - shoulder;
                        float distance = glm::clamp(glm::length(delta), 0.00001f, (upper + lower) * 0.98f);
                        auto direction = glm::normalize(delta);
                        auto pole = glm::vec3(-1.f, -0.65f, 0.15f);
                        auto bend = glm::normalize(pole - direction * glm::dot(pole, direction));
                        float along = (upper * upper - lower * lower + distance * distance) / (2 * distance);
                        waveElbow = shoulder + direction * along + bend * std::sqrt(std::max(0.f, upper * upper - along * along));
                        raised = aimBone(restBasis, waveElbow - shoulder);
                    } else if (joint == 1) {
                        raised = aimBone(restBasis, waveWrist - waveElbow);
                    } else {
                        // An open palm sweeps in front of the face; fingers point upward.
                        raised = glm::angleAxis(-0.5f - wave * 0.45f, glm::vec3(0, 0, 1)) *
                                 glm::angleAxis(-0.35f + wave * 0.25f, glm::vec3(0, 1, 0));
                    }
                }
                orientBone(world, side ? glm::slerp(idle, raised, swatWeight) : idle);
            }
        }
        for (int side = 0; side < 2; ++side) {
            auto const& arm = side ? m->rightArm : m->leftArm;
            auto const& twists = side ? m->rightTwist : m->leftTwist;
            for (int joint = 0; joint < 3; ++joint) {
                if (static_cast<int>(i) != twists[joint]) continue;
                glm::mat3 forearm(m->world[arm[1]]), wrist(m->world[arm[2]]);
                for (int axis = 0; axis < 3; ++axis) {
                    forearm[axis] = glm::normalize(forearm[axis]);
                    wrist[axis] = glm::normalize(wrist[axis]);
                }
                auto expected = glm::quat_cast(forearm) * m->rotations[arm[2]];
                auto delta = glm::normalize(glm::quat_cast(wrist) * glm::inverse(expected));
                auto axis = forearm[1];
                auto parallel = axis * glm::dot(glm::vec3(delta.x, delta.y, delta.z), axis);
                auto twist = glm::quat(delta.w, parallel);
                if (glm::length(twist) > 0.00001f) {
                    auto down = torsoTurn * glm::vec3(side ? -0.12f : 0.12f, -1.f, 0.12f);
                    auto idleElbow = aimBone(torsoTurn * m->restOrientation[arm[1]], down);
                    auto idleWrist = aimBone(torsoTurn * m->restOrientation[arm[2]], down);
                    auto idleDelta = idleWrist * glm::inverse(idleElbow * m->rotations[arm[2]]);
                    auto idleAxis = glm::mat3_cast(idleElbow)[1];
                    float reference = 2 * std::atan2(glm::dot(glm::vec3(idleDelta.x, idleDelta.y, idleDelta.z), idleAxis), idleDelta.w);
                    float angle = 2 * std::atan2(glm::dot(parallel, axis), delta.w);
                    // Keep the same axial turn through 180 degrees instead of switching quaternion hemispheres.
                    float branch = reference + (side ? swatWeight * 3.14159265f : 0.f);
                    angle = branch + std::remainder(angle - branch, 6.2831853f);
                    twist = glm::angleAxis(angle * (joint + 1) / 3.f, axis);
                    auto pivot = glm::vec3(world[3]);
                    world = glm::translate(glm::mat4(1), pivot) * glm::mat4_cast(twist) *
                            glm::translate(glm::mat4(1), -pivot) * world;
                }
            }
        }
        m->world[i] = world;
    }
    float blinkPhase = static_cast<float>(std::fmod(std::max(seconds, 0.0) + 1.4, 5.7));
    float blink = blinkPhase < 0.18f ? std::sin(blinkPhase / 0.18f * 3.14159265f) : 0;
    exitWeight = glm::clamp(exitWeight, 0.f, 1.f);
    for (size_t j = 0; j < meshes.size(); ++j) {
        auto& mesh = meshes[j]; auto& source = m->sources[j];
        std::vector<glm::mat4> skin(source.inverseBind.size());
        for (size_t k = 0; k < skin.size(); ++k)
            skin[k] = m->world[source.node->skin->joints[k] - raw->nodes] * source.inverseBind[k];
        for (size_t v = 0; v < mesh.vertices.size(); ++v) {
            glm::vec4 p(source.positions[v] + source.morphs[0][v] * blink +
                        source.morphs[1][v] * (1 - exitWeight), 1);
            if (source.face && std::binary_search(EyeVertices.begin(), EyeVertices.end(), static_cast<unsigned>(v))) {
                // Iris movement stays independent of the head and fades out under closed eyelids.
                p.x += gaze.x * 0.0045f * (1 - blink);
                p.y += gaze.y * 0.0085f * (1 - blink);
            }
            glm::vec4 out(0);
            for (int k = 0; k < 4; ++k) out += (skin[source.joints[v][k]] * p) * source.weights[v][k];
            // A few author vertices are unweighted; Blender leaves those in mesh space too.
            if (source.weights[v] == glm::vec4(0)) out = m->world[source.node - raw->nodes] * p;
            mesh.vertices[v].position = glm::vec3(out);
        }
    }
}
}
