#include "FrameContext.hpp"

#include "Buffer.hpp"
#include "UniformData.hpp"
#include "VulkanContext.hpp"

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

std::vector<FrameContext> FrameContext::createFrameInFlights(
    const VulkanContext& ctx,
    int fif
) {
    std::vector<FrameContext> frames(fif);
    // create uniform buffers first
    for (int i = 0; i < fif; i++) {
        frames[i].sceneUniformBuf = BufferFactory::createDynamicBuffer(
            BufferFactory::Type::Uniform,
            *ctx.allocator,
            sizeof(DefaultSceneUBO)
        );
    }
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = ctx.commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = static_cast<uint32_t>(fif)
    };
    vk::raii::CommandBuffers commandbufs{ctx.device, allocInfo};

    auto layouts = *ctx.defaultLayouts->setLayouts[0];
    std::vector<vk::DescriptorSetLayout> desSetLayouts(
        fif, layouts
    );
    vk::DescriptorSetAllocateInfo setAllocInfo{
        .descriptorPool = ctx.descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(desSetLayouts.size()),
        .pSetLayouts = desSetLayouts.data()
    };
    vk::raii::DescriptorSets sets{ctx.device, setAllocInfo};

    assert(commandbufs.size() == static_cast<size_t>(fif));
    assert(sets.size() == static_cast<size_t>(fif));

    for (int i = 0; i < fif; i++) {
        frames[i].presentComplete = vk::raii::Semaphore{
            ctx.device, vk::SemaphoreCreateInfo{}
        };
        frames[i].fences = vk::raii::Fence{
            ctx.device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled}
        };
        frames[i].cmdBuffer = std::move(commandbufs[i]);
        frames[i].sceneDescriptorSet = std::move(sets[i]);

        // update DescriptorSets
        vk::DescriptorBufferInfo bufferInfo{
            .buffer = frames[i].sceneUniformBuf->getVkBuffer(),
            .offset = 0,
            .range = sizeof(DefaultSceneUBO)
        };
        std::array descriptorWrites{
            vk::WriteDescriptorSet{
                .dstSet = frames[i].sceneDescriptorSet,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .pBufferInfo = &bufferInfo
            }
        };
        ctx.device.updateDescriptorSets(descriptorWrites, {});
    }

    return frames;
}
