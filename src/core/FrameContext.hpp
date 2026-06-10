#ifndef FRAMECONTEXT_HPP
#define FRAMECONTEXT_HPP

#include <memory>
#include <vulkan/vulkan_raii.hpp>

struct DynamicBuffer;
struct VulkanContext;

struct FrameContext {
    vk::raii::CommandBuffer cmdBuffer{nullptr};
    vk::raii::Semaphore presentComplete{nullptr};
    vk::raii::Fence fences{nullptr};
    vk::raii::DescriptorSet sceneDescriptorSet{nullptr};
    std::shared_ptr<DynamicBuffer> sceneUniformBuf;

    void reset(const vk::raii::Device& device);
    void wait(const vk::raii::Device& device);
    static std::vector<FrameContext> createFrameInFlights(const VulkanContext& ctx, int fif);
};

#endif  // FRAMECONTEXT_HPP
