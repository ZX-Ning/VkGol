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
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

// project
#include "Swapchain.hpp"
#include "VmaBuffer.hpp"
#include "VulkanContext.hpp"
#include "WindowApp.hpp"
#include "backends/imgui_impl_glfw.h"
#include "utils.hpp"
#include "vertex.hpp"

namespace {
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

void transitionImageLayout(
    const vk::Image& image,
    const vk::raii::CommandBuffer& commandBuffer,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    vk::AccessFlags2 src_access_mask,
    vk::AccessFlags2 dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask
) {
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = src_stage_mask,
        .srcAccessMask = src_access_mask,
        .dstStageMask = dst_stage_mask,
        .dstAccessMask = dst_access_mask,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    vk::DependencyInfo dependency_info = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };
    commandBuffer.pipelineBarrier2(dependency_info);
}

vk::raii::ShaderModule createShaderModule(const vk::raii::Device& device, const std::span<char> spv) {
    vk::ShaderModuleCreateInfo createInfo{
        .codeSize = spv.size() * sizeof(char),
        .pCode = reinterpret_cast<const uint32_t*>(spv.data())
    };
    vk::raii::ShaderModule shaderModule{device, createInfo};

    return shaderModule;
}

vk::raii::Pipeline createGraphicsPipeline(
    const vk::raii::Device& device, vk::SurfaceFormatKHR surfaceFormat
) {
    std::vector<char> spv = readFile("shaders/shader.spv");
    vk::raii::ShaderModule shaderModule =
        createShaderModule(device, spv);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = shaderModule,
        .pName = "vertMain"
    };
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = shaderModule,
        .pName = "fragMain"
    };
    vk::PipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageInfo, fragShaderStageInfo
    };

    auto bindingDescription = SimpleVertex::bindingDescription();
    auto attributeDescriptions = SimpleVertex::attributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = attributeDescriptions.size(),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
    };
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        .topology = vk::PrimitiveTopology::eTriangleList
    };
    vk::PipelineViewportStateCreateInfo viewportState{
        .viewportCount = 1, .scissorCount = 1
    };

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eClockwise,
        .depthBiasEnable = vk::False,
        .depthBiasSlopeFactor = 1.0f,
        .lineWidth = 1.0f
    };

    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR |
                          vk::ColorComponentFlagBits::eG |
                          vk::ColorComponentFlagBits::eB |
                          vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = vk::False,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };

    std::vector dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicState{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount = 0, .pushConstantRangeCount = 0
    };

    vk::raii::PipelineLayout pipelineLayout(device, pipelineLayoutInfo);

    vk::StructureChain pipelineCreateInfoChain{
        vk::GraphicsPipelineCreateInfo{
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamicState,
            .layout = pipelineLayout,
            .renderPass = nullptr
        },
        vk::PipelineRenderingCreateInfo{
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &surfaceFormat.format
        }
    };

    return {
        device,
        nullptr,
        pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>()
    };
}

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

std::shared_ptr<StaticBuffer> createVertexBuffer(
    const VmaAllocator& allocator
) {
    const auto& vertices = TRAINGLE;
    // create vertex buffer
    return BufferFactory::createVertexBuffer(allocator, getVectorSize(TRAINGLE));
}

void writeVertexBuffer(
    StaticBuffer& vertBuffer,
    vk::raii::Queue& queue,
    vk::raii::CommandBuffer& cmd
) {
    const auto& vertices = TRAINGLE;
    cmd.begin(
        vk::CommandBufferBeginInfo{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        }
    );
    vertBuffer.load(asRawBytes(TRAINGLE), cmd);
    cmd.end();

    const vk::SubmitInfo submitInfo{
        .commandBufferCount = 1,
        .pCommandBuffers = &*cmd,
    };
    queue.submit(submitInfo);
    queue.waitIdle();
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

// TODO: ADD MORE FUNCTION HERE
}  // namespace

void VulkanApp::init() {
    Size2D<uint32_t> size = windowApp->getFrameSize();
    context.reset(new VulkanContext(*windowApp));
    swapChain.reset(new SwapChain(*context, size));
    graphicsPipeline = createGraphicsPipeline(
        context->device, swapChain->surfaceFormat
    );
    initFrames();
    initImgui();
    vertexBuffer = createVertexBuffer(*(context->allocator));
    writeVertexBuffer(*vertexBuffer, context->queue, context->loadingCmdBuffer);
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
        .DescriptorPoolSize = 1 << 4,
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
            const vk::Instance* instance =
                reinterpret_cast<const vk::Instance*>(data);
            VulkanContext* runningContext = VulkanContext::runningIntance();
            return runningContext
                ->getDispatcher()
                ->vkGetInstanceProcAddr(*instance, name);
        },
        (void*)(&*context->instance)  // using c-style cast for const + reinterpret cast
    );

    ImGui_ImplVulkan_Init(&initInfo);
};

void VulkanApp::initFrames() {
    assert(frames.empty());
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = context->commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };
    vk::raii::CommandBuffers commandbufs{context->device, allocInfo};

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        frames.emplace_back(
            std::move(commandbufs[i]),
            vk::raii::Semaphore{context->device, vk::SemaphoreCreateInfo{}},
            vk::raii::Fence{
                context->device,
                vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled}
            }
        );
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
    auto& frame = frames[frameIndex];

    vk::Result fenceResult = context->device.waitForFences(
        *frame.fences, vk::True, UINT64_MAX
    );
    if (fenceResult != vk::Result::eSuccess) {
        throw std::runtime_error("failed to wait for fence!");
    }
    context->device.resetFences(*frame.fences);

    auto [result, imageIndex] = swapChain->swapChain.acquireNextImage(
        UINT64_MAX,
        *frame.presentComplete,
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

    auto& image = swapChain->images[imageIndex];
    frame.cmdBuffer.reset();
    auto width = static_cast<float>(swapChain->extent.width);
    auto height = static_cast<float>(swapChain->extent.height);

    // record commandBuffer
    {
        frame.cmdBuffer.begin({});
        // Before starting rendering, transition the swapchain image to
        // COLOR_ATTACHMENT_OPTIMAL
        transitionImageLayout(
            image.image,
            frame.cmdBuffer,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            {},  // srcAccessMask (no need to wait for previous operations)
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput
        );
        vk::ClearValue clearColor = {state.clearColor.srgbToLinear()};
        vk::RenderingAttachmentInfo attachmentInfo = vk::RenderingAttachmentInfo{
            .imageView = image.imageView,
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clearColor
        };
        vk::RenderingInfo renderingInfo = {
            .renderArea = {
                .offset = {0, 0},
                .extent = swapChain->extent
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &attachmentInfo
        };
        frame.cmdBuffer.beginRendering(renderingInfo);
        frame.cmdBuffer.setViewport(
            0, vk::Viewport(0.0f, 0.0f, width, height, 0.0f, 1.0f)
        );
        frame.cmdBuffer.setScissor(
            0, vk::Rect2D(vk::Offset2D(0, 0), swapChain->extent)
        );
        frame.cmdBuffer.bindPipeline(
            vk::PipelineBindPoint::eGraphics,
            *graphicsPipeline
        );
        frame.cmdBuffer.bindVertexBuffers(
            0, vertexBuffer->getVkBuffer(), {0}
        );
        frame.cmdBuffer.draw(3, 1, 0, 0);
        drawImgui(frame.cmdBuffer, state);
        frame.cmdBuffer.endRendering();
        // After rendering, transition the swapchain image to PRESENT_SRC
        transitionImageLayout(
            image.image,
            frame.cmdBuffer,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            {},
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eBottomOfPipe
        );
        frame.cmdBuffer.end();
    }

    vk::PipelineStageFlags waitDestinationStageMask(
        vk::PipelineStageFlagBits::eColorAttachmentOutput
    );
    const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*frame.presentComplete,
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*frame.cmdBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*image.renderComplete,
    };
    context->queue.submit(submitInfo, *frame.fences);

    try {
        const vk::PresentInfoKHR presentInfoKHR{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*image.renderComplete,
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
        int width, height;
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
