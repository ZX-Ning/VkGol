#ifndef IMAGEUTILS_HPP
#define IMAGEUTILS_HPP

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

void transitionImageLayout(
    vk::raii::CommandBuffer& cmd,
    vk::Image image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::PipelineStageFlags2 srcStage,
    vk::AccessFlags2 srcAccess,
    vk::PipelineStageFlags2 dstStage,
    vk::AccessFlags2 dstAccess,
    vk::ImageAspectFlags aspectMask
);

#endif  // IMAGEUTILS_HPP
