#include "RenderApp.hpp"

#include "AppState.hpp"

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
#include "ImguiApp.hpp"
#include "ModelLoader.hpp"
#include "Scene.hpp"
#include "WindowApp.hpp"
#include "core/RenderPipeline.hpp"
#include "core/Swapchain.hpp"
#include "core/Texture.hpp"
#include "core/UniformData.hpp"
#include "core/VulkanContext.hpp"
#include "utils.hpp"

namespace {
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

}  // namespace

void RenderApp::init() {
    initFrames();
    state.lastRenderTimestamp = getTimestampMs();
}

void RenderApp::initFrames() {
    assert(frames.empty());
    frames.resize(MAX_FRAMES_IN_FLIGHT);

    // create uniform buffers first
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        frames[i].sceneUniformBuf = BufferFactory::createDynamicBuffer(
            BufferFactory::Type::Uniform,
            *context.allocator,
            sizeof(DefaultScenceUBO)
        );
    }

    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = context.commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };
    vk::raii::CommandBuffers commandbufs{context.device, allocInfo};

    auto layouts = *context.defaultLayouts->setLayouts[0];
    std::vector<vk::DescriptorSetLayout> desSetLayouts(
        MAX_FRAMES_IN_FLIGHT, layouts
    );
    vk::DescriptorSetAllocateInfo setAllocInfo{
        .descriptorPool = context.descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(desSetLayouts.size()),
        .pSetLayouts = desSetLayouts.data()
    };
    vk::raii::DescriptorSets sets{context.device, setAllocInfo};

    assert(commandbufs.size() == MAX_FRAMES_IN_FLIGHT);
    assert(sets.size() == MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        frames[i].presentComplete = vk::raii::Semaphore{
            context.device, vk::SemaphoreCreateInfo{}
        };
        frames[i].fences = vk::raii::Fence{
            context.device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled}
        };
        frames[i].cmdBuffer = std::move(commandbufs[i]);
        frames[i].sceneDescriptorSet = std::move(sets[i]);

        // update DescriptorSets
        vk::DescriptorBufferInfo bufferInfo{
            .buffer = frames[i].sceneUniformBuf->getVkBuffer(),
            .offset = 0,
            .range = sizeof(DefaultScenceUBO)
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
        context.device.updateDescriptorSets(descriptorWrites, {});
    }
}

void RenderApp::updateState() {
    uint64_t timeNow = getTimestampMs();
    state.frameTime = timeNow - state.lastRenderTimestamp;
    state.lastRenderTimestamp = timeNow;
    if (!scene.has_value()) {
        return;
    }
    auto& sceneRef = (*scene).get();
    glm::fvec3 front = glm::normalize(sceneRef.view.front);
    glm::fvec3 up = glm::normalize(sceneRef.view.up);
    glm::fvec3 right = glm::normalize(glm::cross(front, up));

    float displacement = (state.frameTime / 1000.0) * state.moveSpeed;
    if (windowApp.getKeyState(GLFW_KEY_W) == GLFW_PRESS) {
        sceneRef.view.eye += displacement * front;
    }
    if (windowApp.getKeyState(GLFW_KEY_S) == GLFW_PRESS) {
        sceneRef.view.eye -= displacement * front;
    }
    if (windowApp.getKeyState(GLFW_KEY_A) == GLFW_PRESS) {
        sceneRef.view.eye -= displacement * right;
    }
    if (windowApp.getKeyState(GLFW_KEY_D) == GLFW_PRESS) {
        sceneRef.view.eye += displacement * right;
    }
    if (windowApp.getKeyState(GLFW_KEY_SPACE) == GLFW_PRESS) {
        sceneRef.view.eye += displacement * up;
    }
    if (windowApp.getKeyState(GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        sceneRef.view.eye -= displacement * up;
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

    updateState();

    auto size = windowApp.getFrameSize();
    // ImGui::GetIO().DisplaySize = ImVec2{size.width * 1.f, size.height * 1.f};

    // Note: inFlightFences, presentCompleteSemaphores, and commandBuffers
    // are indexed by frameIndex, while renderFinishedSemaphores is indexed by imageIndex
    auto& currentFrame = frames[frameIndex];

    vk::Result fenceResult = context.device.waitForFences(
        *currentFrame.fences, vk::True, UINT64_MAX
    );
    if (fenceResult != vk::Result::eSuccess) {
        throw std::runtime_error("failed to wait for fence!");
    }
    context.device.resetFences(*currentFrame.fences);

    auto [result, imageIndex] = swapChain.swapChain.acquireNextImage(
        UINT64_MAX,
        *currentFrame.presentComplete,
        nullptr
    );

    if (result == vk::Result::eErrorOutOfDateKHR) {
        swapChain.recreate(context, size);
        return;
    }
    if (result != vk::Result::eSuccess &&
        result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    auto& currentImage = swapChain.images[imageIndex];
    auto& currentCmdBuffer = currentFrame.cmdBuffer;
    currentCmdBuffer.reset();
    auto width = static_cast<float>(swapChain.extent.width);
    auto height = static_cast<float>(swapChain.extent.height);
    vk::Viewport viewport{
        0.0f,
        height,
        width,
        -height,
        0.0f,
        1.0f
    };

    // record commandBuffer
    currentCmdBuffer.begin({});
    {
        // Before starting rendering, transition the swapchain image to COLOR_ATTACHMENT_OPTIMAL
        vk::ImageMemoryBarrier2 barrierTop = {
            .srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe,
            .srcAccessMask = {},
            .dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
            .oldLayout = vk::ImageLayout::eUndefined,
            .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = currentImage.image,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        vk::ImageMemoryBarrier2 barrierDepth = {
            .srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe,
            .srcAccessMask = {},
            .dstStageMask =
                vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                vk::PipelineStageFlagBits2::eLateFragmentTests,
            .dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            .oldLayout = vk::ImageLayout::eUndefined,
            .newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = currentImage.depthTexture->image,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eDepth,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        std::array barriers = {barrierTop, barrierDepth};
        currentCmdBuffer.pipelineBarrier2(
            vk::DependencyInfo{
                .dependencyFlags = {},
                .imageMemoryBarrierCount = barriers.size(),
                .pImageMemoryBarriers = barriers.data()
            }
        );
        // Rendering:
        vk::RenderingAttachmentInfo colorAttachmentInfo = vk::RenderingAttachmentInfo{
            .imageView = currentImage.imageView,
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = {
                .color = {state.clearColor.srgbToLinear()}
            }
        };
        vk::RenderingAttachmentInfo depthAttachmentInfo = {
            .imageView = currentImage.depthTexture->view,
            .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eDontCare,
            .clearValue = {
                .depthStencil = vk::ClearDepthStencilValue(1.0f, 0)
            }
        };
        {
            currentCmdBuffer.beginRendering(
                vk::RenderingInfo{
                    .renderArea = {
                        .offset = {0, 0},
                        .extent = swapChain.extent
                    },
                    .layerCount = 1,
                    .colorAttachmentCount = 1,
                    .pColorAttachments = &colorAttachmentInfo,
                    .pDepthAttachment = &depthAttachmentInfo
                }
            );
            currentCmdBuffer.setViewport(0, viewport);
            currentCmdBuffer.setScissor(
                0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.extent)
            );
            currentCmdBuffer.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                context.defaultLayouts->pipelineLayout,
                0,
                *currentFrame.sceneDescriptorSet,
                nullptr
            );

            if (scene.has_value()) {
                auto& sceneRef = (*scene).get();
                sceneRef.render(
                    *context.defaultLayouts,
                    currentCmdBuffer,
                    *(currentFrame.sceneUniformBuf),
                    (1.0 * size.width) / size.height
                );
            }
            imgui.renderVk(imgui.drawImgui(state, *scene), currentCmdBuffer);
            currentCmdBuffer.endRendering();
        }

        // After rendering, transition the swapchain image to PRESENT_SRC
        vk::ImageMemoryBarrier2 barrierBottom = {
            .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
            .dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe,
            .dstAccessMask = {},
            .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .newLayout = vk::ImageLayout::ePresentSrcKHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = currentImage.image,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        currentCmdBuffer.pipelineBarrier2(
            vk::DependencyInfo{
                .dependencyFlags = {},
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers = &barrierBottom
            }
        );
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
        .pSignalSemaphores = &*currentImage.renderComplete,
    };
    context.queue.submit(submitInfo, *currentFrame.fences);

    try {
        const vk::PresentInfoKHR presentInfoKHR{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*currentImage.renderComplete,
            .swapchainCount = 1,
            .pSwapchains = &(*swapChain.swapChain),
            .pImageIndices = &imageIndex
        };
        result = context.queue.presentKHR(presentInfoKHR);
        if (result == vk::Result::eSuboptimalKHR || framebufferResized) {
            framebufferResized = false;
            swapChain.recreate(context, size);
        }
        else if (result != vk::Result::eSuccess) {
            throw std::runtime_error("failed to present swap chain image!");
        }
    }
    catch (const vk::SystemError& e) {
        if (e.code().value() == static_cast<int>(vk::Result::eErrorOutOfDateKHR)) {
            swapChain.recreate(context, size);
            return;
        }
        else {
            throw;
        }
    }

    frameIndex++;
    frameIndex %= MAX_FRAMES_IN_FLIGHT;
}

RenderApp::RenderApp(AppState& state, VulkanContext& context, WindowApp& windowApp, SwapChain& swapChain, ImguiApp& imgui)
    : state(state), context(context), windowApp(windowApp), swapChain(swapChain), imgui(imgui) {
    init();
}

void RenderApp::setScene(Scene& scene) {
    this->scene = scene;
}

RenderApp::~RenderApp() = default;

void RenderApp::run() {
   
    windowApp.run();
}
