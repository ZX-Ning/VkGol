#ifndef MODEL_HPP
#define MODEL_HPP

#include <glm/ext/matrix_float4x4.hpp>
#include <memory>
#include <vulkan/vulkan_raii.hpp>

#include "Buffer.hpp"

struct Pipeline;
struct VulkanContext;
struct Texture;

struct Mesh {
    std::shared_ptr<StaticBuffer> vertexBuffer;
    std::shared_ptr<StaticBuffer> indexBuffer;
    uint32_t indexCount;
};

struct Material {
    std::shared_ptr<Pipeline> pipeline;
    std::shared_ptr<Texture> texture;
    std::shared_ptr<vk::raii::DescriptorSet> descriptorSet;
};

struct Model {
    Mesh mesh;
    Material material;
    void bind(const vk::raii::PipelineLayout&, vk::raii::CommandBuffer&);
    void render(vk::raii::CommandBuffer&);
};

#endif  // MODEL_HPP
