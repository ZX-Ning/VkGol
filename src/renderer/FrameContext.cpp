#include "FrameContext.hpp"

#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

#include "../core/VulkanContext.hpp"

FrameContext::FrameContext(
    const VulkanContext& context,
    vk::raii::CommandBuffer&& commandBuffer
)
    : cmdBuffer(std::move(commandBuffer)),
      presentComplete(context.device, vk::SemaphoreCreateInfo{}),
      fences(
          context.device,
          vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled}
      ) {}

void FrameContext::reset(const vk::raii::Device& device) {
    device.resetFences(*fences);
}

void FrameContext::wait(const vk::raii::Device& device) {
    const vk::Result result = device.waitForFences(
        *fences,
        vk::True,
        UINT64_MAX
    );
    if (result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to wait for frame fence");
    }
}

std::vector<FrameContext> FrameContext::createFrameInFlights(
    const VulkanContext& context,
    uint32_t frameCount
) {
    vk::raii::CommandBuffers commandBuffers{
        context.device,
        vk::CommandBufferAllocateInfo{
            .commandPool = *context.commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = frameCount
        }
    };

    std::vector<FrameContext> frames;
    frames.reserve(frameCount);
    for (vk::raii::CommandBuffer& commandBuffer : commandBuffers) {
        frames.emplace_back(context, std::move(commandBuffer));
    }
    return frames;
}
