#include "FrameContext.hpp"

#include <cassert>
#include <utility>

#include "../core/Buffer.hpp"
#include "../core/Texture.hpp"
#include "../core/VulkanContext.hpp"
#include "../core/VulkanUtils.hpp"
#include "ForwardShaderData.hpp"

FrameContext::FrameContext(
    const VulkanContext& ctx,
    vk::raii::CommandBuffer&& commandBuffer,
    vk::raii::DescriptorSet&& sceneDescriptorSet,
    Size2D<uint32_t> size
)
    : cmdBuffer(std::move(commandBuffer)),
      presentComplete(ctx.device, vk::SemaphoreCreateInfo{}),
      fences(ctx.device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled}),
      sceneDescriptorSet(std::move(sceneDescriptorSet)) {
    sceneUniformBuf = BufferFactory::createDynamicBuffer(
        BufferFactory::Type::Uniform,
        *ctx.allocator,
        sizeof(DefaultSceneUBO)
    );
    recreateTextures(ctx, size);

    vk::DescriptorBufferInfo bufferInfo{
        .buffer = sceneUniformBuf->getVkBuffer(),
        .offset = 0,
        .range = sizeof(DefaultSceneUBO)
    };
    std::array descriptorWrites{
        vk::WriteDescriptorSet{
            .dstSet = sceneDescriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &bufferInfo
        }
    };
    ctx.device.updateDescriptorSets(descriptorWrites, {});
}

void FrameContext::reset(const vk::raii::Device& device) {
    device.resetFences(*fences);
}

void FrameContext::wait(const vk::raii::Device& device) {
    vk::Result fenceResult = device.waitForFences(
        *fences, vk::True, UINT64_MAX
    );
    if (fenceResult != vk::Result::eSuccess) {
        throw std::runtime_error("failed to wait for fence!");
    }
}

void FrameContext::recreateTextures(const VulkanContext& ctx, Size2D<uint32_t> size) {
    vk::Extent3D extent{size.width, size.height, 1};
    depthTexture = createDepthTexture(ctx, extent);
    pingpongTexture[0] = createPingPongTexture(ctx, extent);
    pingpongTexture[1] = createPingPongTexture(ctx, extent);
}

void FrameContext::transitionDepthToAttachment(vk::raii::CommandBuffer& cmd) {
    transitionImageLayout(
        cmd,
        depthTexture->image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthAttachmentOptimal,
        vk::PipelineStageFlagBits2::eTopOfPipe,
        {},
        vk::PipelineStageFlagBits2::eEarlyFragmentTests |
            vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::ImageAspectFlagBits::eDepth
    );
}

std::vector<FrameContext> FrameContext::createFrameInFlights(
    const VulkanContext& ctx,
    int fif,
    Size2D<uint32_t> size,
    vk::DescriptorSetLayout sceneSetLayout
) {
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = ctx.commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = static_cast<uint32_t>(fif)
    };
    vk::raii::CommandBuffers commandbufs{ctx.device, allocInfo};

    std::vector<vk::DescriptorSetLayout> desSetLayouts(
        fif, sceneSetLayout
    );
    vk::DescriptorSetAllocateInfo setAllocInfo{
        .descriptorPool = ctx.descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(desSetLayouts.size()),
        .pSetLayouts = desSetLayouts.data()
    };
    vk::raii::DescriptorSets sets{ctx.device, setAllocInfo};

    assert(commandbufs.size() == static_cast<size_t>(fif));
    assert(sets.size() == static_cast<size_t>(fif));

    std::vector<FrameContext> frames;
    frames.reserve(fif);
    for (int i = 0; i < fif; i++) {
        frames.emplace_back(
            ctx,
            std::move(commandbufs[i]),
            std::move(sets[i]),
            size
        );
    }

    return frames;
}
