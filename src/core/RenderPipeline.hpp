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

struct GraphicsPipelineDesc {
    vk::PipelineLayout layout;
    std::span<const uint8_t> shaderSpv;
    vk::PipelineVertexInputStateCreateInfo vertexInfo;
    vk::Format colorFormat;
    vk::Format depthFormat;
};

std::shared_ptr<Pipeline> createGraphicsPipeline(
    const VulkanContext& context,
    const GraphicsPipelineDesc& desc
);

#endif  // RENDERPIPELINE_HPP
