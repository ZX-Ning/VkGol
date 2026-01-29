#ifndef RENDERPIPELINE_HPP
#define RENDERPIPELINE_HPP

#include <memory>
#include <span>
#include <vulkan/vulkan_raii.hpp>
// #include "VulkanContext.hpp"

class VulkanContext;

struct Pipeline {
    vk::raii::Pipeline pipeline{nullptr};
    vk::raii::PipelineLayout layout{nullptr};
};


std::shared_ptr<Pipeline> createDefaultGraphicsPipeline(
    const VulkanContext& context,
    std::span<const uint8_t> shaderSpv,
    vk::PipelineVertexInputStateCreateInfo vertexInfo
);

#endif  // RENDERPIPELINE_HPP
