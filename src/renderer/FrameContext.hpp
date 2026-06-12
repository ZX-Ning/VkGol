#ifndef FRAMECONTEXT_HPP
#define FRAMECONTEXT_HPP

#include <array>
#include <memory>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "../core/Texture.hpp"
#include "../utils.hpp"

struct DynamicBuffer;
struct VulkanContext;

struct FrameContext {
    vk::raii::CommandBuffer cmdBuffer{nullptr};
    vk::raii::Semaphore presentComplete{nullptr};
    vk::raii::Fence fences{nullptr};
    vk::raii::DescriptorSet sceneDescriptorSet{nullptr};
    std::array<std::unique_ptr<Texture>, 2> pingpongTexture;
    std::shared_ptr<DynamicBuffer> sceneUniformBuf;
    std::unique_ptr<Texture> depthTexture;

    FrameContext(
        const VulkanContext& ctx,
        vk::raii::CommandBuffer&& commandBuffer,
        vk::raii::DescriptorSet&& sceneSet,
        Size2D<uint32_t> size
    );

    void reset(const vk::raii::Device& device);
    void wait(const vk::raii::Device& device);
    void recreateTextures(const VulkanContext& ctx, Size2D<uint32_t> size);
    void transitionDepthToAttachment(vk::raii::CommandBuffer& cmd);

    static std::vector<FrameContext> createFrameInFlights(
        const VulkanContext& ctx,
        int fif,
        Size2D<uint32_t> size,
        vk::DescriptorSetLayout sceneSetLayout
    );
};

#endif  // FRAMECONTEXT_HPP
