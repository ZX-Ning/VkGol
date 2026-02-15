#ifndef RENDERPIPELINE_HPP
#define RENDERPIPELINE_HPP

#include <memory>
#include <span>
#include <vulkan/vulkan_raii.hpp>
// #include "VulkanContext.hpp"

struct VulkanContext;

struct Pipeline {
    vk::raii::Pipeline pipeline{nullptr};
};

std::shared_ptr<Pipeline> createDefaultGraphicsPipeline(
    const VulkanContext& context,
    std::span<const uint8_t> shaderSpv,
    vk::PipelineVertexInputStateCreateInfo vertexInfo
);

#endif  // RENDERPIPELINE_HPP
