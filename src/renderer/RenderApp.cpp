#include "RenderApp.hpp"

#include "../AppState.hpp"

// std c++
#include <cassert>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

// vulkan
#include <vulkan/vulkan_core.h>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_structs.hpp>
#include <vulkan/vulkan_to_string.hpp>

// project
#include "../ImguiApp.hpp"
#include "../WindowApp.hpp"
#include "../core/Swapchain.hpp"
#include "../core/VulkanContext.hpp"
#include "../utils.hpp"
#include "ForwardRenderLayout.hpp"
#include "Scene.hpp"

namespace {
constexpr int MAX_FRAMES_IN_FLIGHT = 2;
}  // namespace

void RenderApp::init() {
    frames = FrameContext::createFrameInFlights(
        context,
        MAX_FRAMES_IN_FLIGHT,
        {swapChain.extent.width, swapChain.extent.height},
        *layout.sceneSetLayout
    );
    state.lastRenderTimestamp = getTimestampMs();
}

void RenderApp::recreateSwapChainResources(Size2D<uint32_t> size) {
    swapChain.recreate(context, size);
    Size2D<uint32_t> targetSize{swapChain.extent.width, swapChain.extent.height};
    for (auto& frame : frames) {
        frame.recreateTextures(context, targetSize);
    }
}

void RenderApp::drawFrame() {
    if (state.quit) {
        return;
    }
    if (windowApp.isMinimized()) {
        this->framebufferResized = true;
        // std::println("Minimized, skip rendering");
        return;
    }

    auto size = windowApp.getFrameSize();
    // ImGui::GetIO().DisplaySize = ImVec2{size.width * 1.f, size.height * 1.f};

    // Note: inFlightFences, presentCompleteSemaphores, and commandBuffers
    // are indexed by frameIndex, while renderFinishedSemaphores is indexed by imageIndex
    auto& currentFrame = frames[frameIndex];

    currentFrame.wait(context.device);
    uint32_t imageIndex = 0;
    try {
        imageIndex = swapChain.acquireNextImage(currentFrame.presentComplete);
    }
    catch (SwapChain::SwapChainOutOfDateError&) {
        recreateSwapChainResources(size);
        return;
    }
    currentFrame.reset(context.device);

    auto& currentImage = swapChain.images[imageIndex];
    auto& currentCmdBuffer = currentFrame.cmdBuffer;
    currentCmdBuffer.reset();

    currentCmdBuffer.begin({});
    {
        currentImage.transitionToColorAttachment(currentCmdBuffer);
        currentFrame.transitionDepthToAttachment(currentCmdBuffer);
        forwardPass.record(
            ForwardPassContext{
                .context = context,
                .layout = layout,
                .frame = currentFrame,
                .target = currentImage,
                .extent = swapChain.extent,
                .windowSize = size,
                .state = state,
                .imgui = imgui
            }
        );
        currentImage.transitionToPresent(currentCmdBuffer);
        currentCmdBuffer.end();
    }

    vk::PipelineStageFlags waitDestinationStageMask(
        vk::PipelineStageFlagBits::eColorAttachmentOutput
    );
    const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*currentFrame.presentComplete,
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*currentFrame.cmdBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*currentImage.imageAcquired,
    };
    context.queue.submit(submitInfo, *currentFrame.fences);

    try {
        const vk::PresentInfoKHR presentInfoKHR{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*currentImage.imageAcquired,
            .swapchainCount = 1,
            .pSwapchains = &(*swapChain.vkSwapChain),
            .pImageIndices = &imageIndex
        };
        auto result = context.queue.presentKHR(presentInfoKHR);
        if (result == vk::Result::eSuboptimalKHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChainResources(size);
        }
        else if (result != vk::Result::eSuccess) {
            throw std::runtime_error("failed to present swap chain image!");
        }
    }
    catch (const vk::SystemError& e) {
        if (e.code().value() == static_cast<int>(vk::Result::eErrorOutOfDateKHR)) {
            recreateSwapChainResources(size);
            return;
        }
        else {
            throw;
        }
    }

    frameIndex++;
    frameIndex %= MAX_FRAMES_IN_FLIGHT;
}

RenderApp::RenderApp(
    AppState& state,
    VulkanContext& context,
    ForwardRenderLayout& layout,
    WindowApp& windowApp,
    SwapChain& swapChain,
    ImguiApp& imgui
)
    : state(state),
      context(context),
      layout(layout),
      windowApp(windowApp),
      swapChain(swapChain),
      imgui(imgui) {
    init();
}

RenderApp::~RenderApp() = default;

void RenderApp::run() {
    windowApp.run();
}
