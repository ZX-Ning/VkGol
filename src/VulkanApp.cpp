#include "VulkanApp.hpp"

// std c++
#include <cassert>
#include <cstdint>
#include <cstring>
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

// imgui
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

// project
#include "SimpleModel.hpp"
#include "WindowApp.hpp"
#include "core/RenderPipeline.hpp"
#include "core/Swapchain.hpp"
#include "core/UniformData.hpp"
#include "core/VulkanContext.hpp"
#include "utils.hpp"

namespace {
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

uint32_t findMemoryType(
    const vk::raii::PhysicalDevice& physicalDevice,
    uint32_t typeFilter,
    vk::MemoryPropertyFlags properties
) {
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void drawImgui(vk::raii::CommandBuffer& buffer, VulkanApp::AppState& state) {
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
    {
        auto imguiScale = ImGui::GetIO().DisplayFramebufferScale;
        auto size = ImGui::GetMainViewport()->Size;
        // std::println("Imgui scale: {}x{}", imguiScale.x, imguiScale.y);
        ImGui::SetNextWindowSize(ImVec2(size.x * 0.75, size.y * 0.15), ImGuiCond_Appearing);
        ImGui::Begin("Hello, world!");
        ImGui::ColorEdit3(
            "clear color",
            state.clearColor.getRaw()
        );
        ImGui::SameLine();
        ImGui::Checkbox("Demo Window", &state.showDemoWindow);
        ImGui::Text("Frame time: %.2fms", state.frameTime);
        ImGui::End();
    }
    if (state.showDemoWindow) {
        ImGui::ShowDemoWindow(&(state.showDemoWindow));
    }
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(draw_data, *buffer);
}

}  // namespace

void VulkanApp::init() {
    Size2D<uint32_t> size = windowApp->getFrameSize();
    context.reset(new VulkanContext(*windowApp));
    swapChain.reset(new SwapChain(*context, size));
    initFrames();
    initImgui();
    model = loadSimpleTraingleModel(*context);
    state.lastRenderTimestamp = getTimestampMs();
}

void VulkanApp::initImgui() {
    ImGuiIO* imguiIo = &ImGui::GetIO();
    imguiIo->ConfigFlags |= ImGuiConfigFlags_IsSRGB;
    imguiIo->IniFilename = NULL;

    // font
    ImFontConfig fontcfg;
    fontcfg.OversampleH = 2;
    fontcfg.OversampleV = 2;
    fontcfg.PixelSnapH = true;
    fontcfg.PixelSnapV = true;
    fontcfg.RasterizerDensity = 0.86f;
    imguiIo->Fonts->AddFontFromFileTTF(
        "assets/fonts/IBMPlex/IBMPlexSans-Regular.ttf",
        17.f,
        &fontcfg
    );
    ImGuiStyle* style = &ImGui::GetStyle();
    auto size = windowApp->getFrameSize();
    // set style
    ImGui::StyleColorsDark();
    style->WindowRounding = 5.f;
    style->FrameRounding = 5.f;
    style->WindowPadding = {8, 5};
    style->FramePadding = {3, 2};

    auto [scaleType, scale] = windowApp->getScale();
    if (scaleType == WindowApp::WINDOWS_OR_X11) {
        style->FontScaleDpi = scale;
        style->ScaleAllSizes(scale);
    }

    vk::PipelineRenderingCreateInfoKHR pipelineInfo{
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapChain->surfaceFormat.format,
    };

    auto vert = readFile("shaders/imgui/vert.spv");
    vk::ShaderModuleCreateInfo vertInfo{
        .codeSize = getVectorSize(vert),
        .pCode = reinterpret_cast<uint32_t*>(vert.data())
    };
    auto frag = readFile("shaders/imgui/frag.spv");
    vk::ShaderModuleCreateInfo fragInfo{
        .codeSize = getVectorSize(frag),
        .pCode = reinterpret_cast<uint32_t*>(frag.data())
    };

    ImGui_ImplVulkan_InitInfo initInfo{
        .ApiVersion = VK_API_VERSION_1_3,
        .Instance = *context->instance,
        .PhysicalDevice = *context->physicalDevice,
        .Device = *context->device,
        .QueueFamily = context->queueFamilyIndex,
        .Queue = *context->queue,
        .DescriptorPoolSize = 0xff,
        .MinImageCount = swapChain->minImageCount,
        .ImageCount = static_cast<uint32_t>(swapChain->images.size()),
        .PipelineInfoMain = {
            .PipelineRenderingCreateInfo = *pipelineInfo,
        },
        .UseDynamicRendering = true,
        .CustomShaderVertCreateInfo = vertInfo,
        .CustomShaderFragCreateInfo = fragInfo
    };

    ImGui_ImplVulkan_LoadFunctions(
        VK_API_VERSION_1_3,
        [](const char* name, void* data) {
            auto instance =
                reinterpret_cast<const vk::raii::Instance*>(data);
            return instance
                ->getDispatcher()
                ->vkGetInstanceProcAddr(**instance, name);
        },
        (void*)(&context->instance)  // using c-style cast for const + reinterpret cast
    );

    ImGui_ImplVulkan_Init(&initInfo);
}

void VulkanApp::initFrames() {
    assert(frames.empty());
    frames.resize(MAX_FRAMES_IN_FLIGHT);

    // create uniform buffers first
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        frames[i].sceneUniformBuf = BufferFactory::createDynamicBuffer(
            BufferFactory::Type::Uniform,
            *context->allocator,
            sizeof(DefaultScenceUBO)
        );
    }

    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = context->commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };
    vk::raii::CommandBuffers commandbufs{context->device, allocInfo};

    auto layouts = *context->set0Layout;
    std::vector<vk::DescriptorSetLayout> desSetLayouts(
        MAX_FRAMES_IN_FLIGHT, layouts
    );
    vk::DescriptorSetAllocateInfo setAllocInfo{
        .descriptorPool = context->descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(desSetLayouts.size()),
        .pSetLayouts = desSetLayouts.data()
    };
    vk::raii::DescriptorSets sets{context->device, setAllocInfo};

    assert(commandbufs.size() == MAX_FRAMES_IN_FLIGHT);
    assert(sets.size() == MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        frames[i].presentComplete = vk::raii::Semaphore{
            context->device, vk::SemaphoreCreateInfo{}
        };
        frames[i].fences = vk::raii::Fence{
            context->device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled}
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
        context->device.updateDescriptorSets(descriptorWrites, {});
    }
}

void VulkanApp::drawFrame() {
    if (windowApp->isMinimized()) {
        this->framebufferResized = true;
        // std::println("Minimized, skip rendering");
        return;
    }
    auto size = windowApp->getFrameSize();
    ImGui::GetIO().DisplaySize = ImVec2{size.width * 1.f, size.height * 1.f};
    uint64_t timeNow = getTimestampMs();
    state.frameTime = timeNow - state.lastRenderTimestamp;
    state.lastRenderTimestamp = timeNow;
    // Note: inFlightFences, presentCompleteSemaphores, and commandBuffers
    // are indexed by frameIndex, while renderFinishedSemaphores is indexed by imageIndex
    auto& currentFrame = frames[frameIndex];

    vk::Result fenceResult = context->device.waitForFences(
        *currentFrame.fences, vk::True, UINT64_MAX
    );
    if (fenceResult != vk::Result::eSuccess) {
        throw std::runtime_error("failed to wait for fence!");
    }
    context->device.resetFences(*currentFrame.fences);

    // update ubo
    DefaultScenceUBO ubo{glm::fmat4(1), glm::fmat4(1)};
    currentFrame.sceneUniformBuf->update(objectAsRawBytes(ubo));

    auto [result, imageIndex] = swapChain->swapChain.acquireNextImage(
        UINT64_MAX,
        *currentFrame.presentComplete,
        nullptr
    );

    if (result == vk::Result::eErrorOutOfDateKHR) {
        swapChain->recreate(*context, size);
        return;
    }
    if (result != vk::Result::eSuccess &&
        result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    auto& currentImage = swapChain->images[imageIndex];
    auto& currentCmdBuffer = currentFrame.cmdBuffer;
    currentCmdBuffer.reset();
    auto width = static_cast<float>(swapChain->extent.width);
    auto height = static_cast<float>(swapChain->extent.height);

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
        currentCmdBuffer.pipelineBarrier2(
            vk::DependencyInfo{
                .dependencyFlags = {},
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers = &barrierTop
            }
        );

        // Rendering:
        vk::RenderingAttachmentInfo attachmentInfo = vk::RenderingAttachmentInfo{
            .imageView = currentImage.imageView,
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = {state.clearColor.srgbToLinear()}
        };
        {
            currentCmdBuffer.beginRendering(
                vk::RenderingInfo{
                    .renderArea = {
                        .offset = {0, 0},
                        .extent = swapChain->extent
                    },
                    .layerCount = 1,
                    .colorAttachmentCount = 1,
                    .pColorAttachments = &attachmentInfo
                }
            );
            currentCmdBuffer.setViewport(
                0,
                vk::Viewport{
                    0.0f,
                    0.0f,
                    width,
                    height,
                    0.0f,
                    1.0f
                }
            );
            currentCmdBuffer.setScissor(
                0, vk::Rect2D(vk::Offset2D(0, 0), swapChain->extent)
            );
            currentCmdBuffer.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                model.material.pipeline->layout,
                0,
                *currentFrame.sceneDescriptorSet,
                nullptr
            );
            model.bind(currentCmdBuffer);
            auto& layout = model.material.pipeline->layout;
            vk::ArrayProxy<const DefaultPushConstant> constants{{glm::fmat4{1.f}}};
            currentCmdBuffer.pushConstants(
                layout, vk::ShaderStageFlagBits::eAllGraphics, 0, constants
            );
            model.render(currentCmdBuffer);

            drawImgui(currentCmdBuffer, state);
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
    context->queue.submit(submitInfo, *currentFrame.fences);

    try {
        const vk::PresentInfoKHR presentInfoKHR{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*currentImage.renderComplete,
            .swapchainCount = 1,
            .pSwapchains = &(*swapChain->swapChain),
            .pImageIndices = &imageIndex
        };
        result = context->queue.presentKHR(presentInfoKHR);
        if (result == vk::Result::eSuboptimalKHR || framebufferResized) {
            framebufferResized = false;
            swapChain->recreate(*context, size);
        }
        else if (result != vk::Result::eSuccess) {
            throw std::runtime_error("failed to present swap chain image!");
        }
    }
    catch (const vk::SystemError& e) {
        if (e.code().value() == static_cast<int>(vk::Result::eErrorOutOfDateKHR)) {
            swapChain->recreate(*context, size);
            return;
        }
        else {
            throw;
        }
    }

    frameIndex++;
    frameIndex %= MAX_FRAMES_IN_FLIGHT;
}

VulkanApp::VulkanApp(std::unique_ptr<WindowApp>&& window)
    : windowApp(std::move(window)) {
    init();
}

VulkanApp::~VulkanApp() = default;

void VulkanApp::run() {
    windowApp->cleanupCallBack =
        [this]() {
            this->context->device.waitIdle();
            ImGui_ImplVulkan_Shutdown();
        };
    windowApp->resizeCallBack =
        [this](int, int) { this->framebufferResized = true; };
    windowApp->drawFrameCallBack =
        [this]() { this->drawFrame(); };
    windowApp->run();
}
