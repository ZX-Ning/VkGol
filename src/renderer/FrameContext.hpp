#ifndef FRAMECONTEXT_HPP
#define FRAMECONTEXT_HPP

#include <vector>

#include <vulkan/vulkan_raii.hpp>

struct VulkanContext;

struct FrameContext {
    vk::raii::CommandBuffer cmdBuffer{nullptr};
    vk::raii::Semaphore presentComplete{nullptr};
    vk::raii::Fence fences{nullptr};

    FrameContext(
        const VulkanContext& context,
        vk::raii::CommandBuffer&& commandBuffer
    );

    void reset(const vk::raii::Device& device);
    void wait(const vk::raii::Device& device);

    static std::vector<FrameContext> createFrameInFlights(
        const VulkanContext& context,
        uint32_t frameCount
    );
};

#endif  // FRAMECONTEXT_HPP
