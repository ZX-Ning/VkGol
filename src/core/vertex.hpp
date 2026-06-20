#ifndef VERTEX_HPP
#define VERTEX_HPP

#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_structs.hpp>

struct SimpleVertex {
    glm::fvec3 pos;
    glm::fvec4 color;
    glm::fvec2 texcoord;

    static vk::VertexInputBindingDescription bindingDescription() {
        return {0, sizeof(SimpleVertex), vk::VertexInputRate::eVertex};
    }

    static std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription{
                0, 0, vk::Format::eR32G32B32Sfloat, offsetof(SimpleVertex, pos)
            },
            vk::VertexInputAttributeDescription{
                1, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(SimpleVertex, color)
            },
            vk::VertexInputAttributeDescription{
                2, 0, vk::Format::eR32G32Sfloat, offsetof(SimpleVertex, texcoord)
            }
        };
    }
};

const std::vector<SimpleVertex> fullScreenQuadVertices = {
    {{-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
    {{1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
    {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
    {{-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
    {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
    {{-1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}
};

#endif
