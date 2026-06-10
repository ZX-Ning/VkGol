#ifndef SCENE_HPP
#define SCENE_HPP

#include <glm/gtc/quaternion.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "../Camera.hpp"
#include "../utils.hpp"

struct Model;
struct DynamicBuffer;
struct AppState;

struct RenderObject {
    Model& model;
    glm::fvec3 position;
    glm::fvec3 scale;
    // rotation
    float angle;
    glm::vec3 axis;

    glm::fmat4x4 calcModelMatrix() const;
    void render(const vk::raii::PipelineLayout&, vk::raii::CommandBuffer& cmd) const;
};

struct Scene {
    std::vector<Ref<RenderObject>> objects;
    View view;
    Camera camera;
    AppState& state;

    Scene(std::vector<Ref<RenderObject>>& objs, AppState& state);

    void render(
        const vk::raii::PipelineLayout&,
        vk::raii::CommandBuffer& cmd,
        DynamicBuffer& ub,
        float ratio
    );
    void resetCamera();
};

#endif  // SCENE_HPP
