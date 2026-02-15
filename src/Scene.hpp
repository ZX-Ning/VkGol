#ifndef SCENE_HPP
#define SCENE_HPP

#include <glm/gtc/quaternion.hpp>
#include <vulkan/vulkan_raii.hpp>

// #include "AppState.hpp"
#include "Camera.hpp"
#include "utils.hpp"

struct Model;
struct DynamicBuffer;
struct VulkanContext;
struct Layouts;

struct RenderObject {
    Model& model;
    glm::fvec3 position;
    glm::fvec3 scale;
    // rotation
    float angle;
    glm::vec3 axis;

    glm::fmat4x4 calcModelMatrix() const;
    void render(const Layouts&, vk::raii::CommandBuffer& cmd) const;
};

struct Scene {
    std::vector<Ref<RenderObject>> objects;
    View view;
    Camera camera;

    void render(
        const Layouts&,
        vk::raii::CommandBuffer& cmd,
        DynamicBuffer& ub,
        float ratio
    );
};

#endif  // SCENE_HPP
