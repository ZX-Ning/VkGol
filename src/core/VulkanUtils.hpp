#ifndef VULKANUTILS_HPP
#define VULKANUTILS_HPP

#include <vk_mem_alloc.h>

#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

struct AllocatorDeleter {
    void operator()(VmaAllocator* ptr) const noexcept {
        if (ptr) {
            vmaDestroyAllocator(*ptr);
        }
    }
};
using VmaAllocatorWrapper = std::unique_ptr<VmaAllocator, AllocatorDeleter>;

inline vk::Format formatToSrgb(vk::Format format) {
    switch (format) {
        case vk::Format::eB8G8R8A8Srgb:
        case vk::Format::eB8G8R8A8Unorm: {
            return vk::Format::eB8G8R8A8Srgb;
        }
        case vk::Format::eR8G8B8A8Srgb:
        case vk::Format::eR8G8B8A8Unorm: {
            return vk::Format::eR8G8B8A8Srgb;
        }
        default: {
            throw std::runtime_error("Format not supported yet.");
        }
    }
}

inline vk::Format formatToUnorm(vk::Format format) {
    switch (format) {
        case vk::Format::eB8G8R8A8Srgb:
        case vk::Format::eB8G8R8A8Unorm: {
            return vk::Format::eB8G8R8A8Unorm;
        }
        case vk::Format::eR8G8B8A8Srgb:
        case vk::Format::eR8G8B8A8Unorm: {
            return vk::Format::eR8G8B8A8Unorm;
        }
        default: {
            throw std::runtime_error("Format not supported yet.");
        }
    }
}

inline void transitionImageLayout(
    vk::raii::CommandBuffer& cmd,
    vk::Image image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::PipelineStageFlags2 srcStage,
    vk::AccessFlags2 srcAccess,
    vk::PipelineStageFlags2 dstStage,
    vk::AccessFlags2 dstAccess,
    vk::ImageAspectFlags aspectMask
) {
    vk::ImageMemoryBarrier2 barrier{
        .srcStageMask = srcStage,
        .srcAccessMask = srcAccess,
        .dstStageMask = dstStage,
        .dstAccessMask = dstAccess,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    cmd.pipelineBarrier2(
        vk::DependencyInfo{
            .dependencyFlags = {},
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier
        }
    );
}

#endif  // VULKANUTILS_HPP
