#include "RenderApp.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <random>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "../AppState.hpp"
#include "../Consts.hpp"
#include "../ImguiApp.hpp"
#include "../WindowApp.hpp"
#include "../core/Buffer.hpp"
#include "../core/RenderPipeline.hpp"
#include "../core/Swapchain.hpp"
#include "../core/Texture.hpp"
#include "../core/VulkanContext.hpp"
#include "../core/vertex.hpp"
#include "GOLPass.hpp"

namespace {

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
constexpr float QUAD_EXTENT_RATIO = 0.95f;

std::mt19937& randomGenerator() {
    static std::random_device rd;
    static std::mt19937 generator(rd());
    return generator;
}

vk::Viewport makeViewport(vk::Extent2D extent) {
    return {
        0.0f,
        static_cast<float>(extent.height),
        static_cast<float>(extent.width),
        -static_cast<float>(extent.height),
        0.0f,
        1.0f
    };
}

glm::mat4 makeSquareTransform(vk::Extent2D extent, uint32_t golSize) {
    const float width = static_cast<float>(extent.width);
    const float height = static_cast<float>(extent.height);
    const float side = QUAD_EXTENT_RATIO * std::min(width, height);

    // Keep every GOL cell aligned to the same integer number of framebuffer
    // pixels. Disabled for now so the quad always occupies exactly 85% of the
    // shorter framebuffer edge.
    // const uint32_t desiredSide = static_cast<uint32_t>(
    //     QUAD_EXTENT_RATIO * std::min(extent.width, extent.height)
    // );
    // const uint32_t cellPixels = std::max(1u, desiredSide / golSize);
    // const float side = static_cast<float>(cellPixels * golSize);
    (void)golSize;

    glm::mat4 transform{1.0f};
    transform[0][0] = side / width;
    transform[1][1] = side / height;
    return transform;
}

}  // namespace

void RenderApp::init() {
    frames = FrameContext::createFrameInFlights(
        context,
        MAX_FRAMES_IN_FLIGHT
    );

    resetGOL(static_cast<uint32_t>(state.golSize));

    const vk::DescriptorSetLayoutBinding textureBinding{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment
    };
    textureSetLayout = context.device.createDescriptorSetLayout(
        vk::DescriptorSetLayoutCreateInfo{}.setBindings(textureBinding)
    );

    const vk::PushConstantRange transformRange{
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .offset = 0,
        .size = sizeof(glm::mat4)
    };
    pipelineLayout = context.device.createPipelineLayout(
        vk::PipelineLayoutCreateInfo{}
            .setSetLayouts({*textureSetLayout})
            .setPushConstantRanges(transformRange)
    );

    const auto bindingDescription = SimpleVertex::bindingDescription();
    const auto attributeDescriptions = SimpleVertex::attributeDescriptions();
    const vk::PipelineVertexInputStateCreateInfo vertexInput{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount =
            static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
    };
    renderPipeline = createGraphicsPipeline(
        context,
        {
            .layout = *pipelineLayout,
            .shaderSpv = readFile(Consts::SHADER_SPV_PATH),
            .vertexInfo = vertexInput,
            .colorFormat = swapChain.surfaceFormat.format,
            .depthFormat = vk::Format::eUndefined,
        }
    );

    const size_t vertexBufferSize = fullScreenQuadVertices.size() * sizeof(SimpleVertex);
    quadVertexBuffer = BufferFactory::createStaticBuffer(
        BufferFactory::Type::Vertex,
        *context.allocator,
        vertexBufferSize
    );
    quadVertexBuffer->loadSync(
        {
            reinterpret_cast<const uint8_t*>(fullScreenQuadVertices.data()),
            vertexBufferSize,
        },
        context
    );

    auto sets = context.device.allocateDescriptorSets(
        {
            .descriptorPool = *context.descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &*textureSetLayout,
        }
    );
    textureSet = std::move(sets[0]);

    updateGOLTextureDescriptor();
}

void RenderApp::resetGOL(uint32_t size) {
    golSize = size;
    auto newPass = std::make_unique<GOLPass>(context, golSize);

    std::bernoulli_distribution aliveDistribution(0.1);
    std::vector<int8_t> initialData(static_cast<size_t>(golSize) * golSize);
#pragma omp parallel for
    for (int8_t& cell : initialData) {
        cell = aliveDistribution(randomGenerator()) ? 1 : 0;
    }
    newPass->initGOLData(context, initialData);
    golPass = std::move(newPass);
}

void RenderApp::updateGOLTextureDescriptor() {
    const Texture& golTexture = golPass->getTexture();
    const vk::DescriptorImageInfo imageInfo{
        .sampler = *golTexture.sampler,
        .imageView = *golTexture.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };
    context.device.updateDescriptorSets(
        {vk::WriteDescriptorSet{
            .dstSet = *textureSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &imageInfo
        }},
        {}
    );
}

void RenderApp::recreateSwapChainResources(Size2D<uint32_t> size) {
    swapChain.recreate(context, size);
}

void RenderApp::record(vk::raii::CommandBuffer& cmd, SurfaceImage& target, vk::Extent2D extent) {
    const vk::RenderingAttachmentInfo colorAttachment{
        .imageView = *target.imageView,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = {
            .color = {state.clearColor.srgbToLinear()}
        }
    };
    cmd.beginRendering(
        vk::RenderingInfo{
            .renderArea = {
                .offset = {0, 0},
                .extent = extent
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment
        }
    );

    cmd.setViewport(0, makeViewport(extent));
    cmd.setScissor(0, vk::Rect2D{{0, 0}, extent});
    cmd.bindPipeline(
        vk::PipelineBindPoint::eGraphics,
        renderPipeline->pipeline
    );
    cmd.bindVertexBuffers(
        0,
        quadVertexBuffer->getVkBuffer(),
        {0}
    );
    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        *pipelineLayout,
        0,
        {*textureSet},
        {}
    );

    const glm::mat4 transform = makeSquareTransform(extent, golSize);
    cmd.pushConstants(
        *pipelineLayout,
        vk::ShaderStageFlagBits::eVertex,
        0,
        vk::ArrayProxy<const glm::mat4>{1, &transform}
    );
    cmd.draw(
        static_cast<uint32_t>(fullScreenQuadVertices.size()),
        1,
        0,
        0
    );

    imgui.renderVk(imgui.drawImgui(state), cmd);
    cmd.endRendering();
}

void RenderApp::drawFrame() {
    if (state.quit || windowApp.isMinimized()) {
        framebufferResized |= windowApp.isMinimized();
        return;
    }

    if (state.resetGOLRequested) {
        context.device.waitIdle();
        state.golSize = (state.golSize / 16) * 16;
        resetGOL(static_cast<uint32_t>(state.golSize));
        updateGOLTextureDescriptor();
        state.resetGOLRequested = false;
    }

    const auto framebufferSize = windowApp.getFrameSize();
    const Size2D<uint32_t> windowSize{
        static_cast<uint32_t>(framebufferSize.width),
        static_cast<uint32_t>(framebufferSize.height)
    };
    FrameContext& currentFrame = frames[frameIndex];
    currentFrame.wait(context.device);

    uint32_t imageIndex;
    try {
        imageIndex = swapChain.acquireNextImage(currentFrame.presentComplete);
    }
    catch (const SwapChain::SwapChainOutOfDateError&) {
        recreateSwapChainResources(windowSize);
        return;
    }

    currentFrame.reset(context.device);
    SurfaceImage& currentImage = swapChain.images[imageIndex];
    vk::raii::CommandBuffer& cmd = currentFrame.cmdBuffer;
    cmd.reset();
    cmd.begin({});

    golPass->record(cmd);
    currentImage.transitionToColorAttachment(cmd);
    record(cmd, currentImage, swapChain.extent);
    currentImage.transitionToPresent(cmd);
    cmd.end();

    const vk::PipelineStageFlags waitStage =
        vk::PipelineStageFlagBits::eColorAttachmentOutput;
    const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*currentFrame.presentComplete,
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &*cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*currentImage.imageAcquired
    };
    context.queue.submit(submitInfo, *currentFrame.fences);

    try {
        const vk::PresentInfoKHR presentInfo{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*currentImage.imageAcquired,
            .swapchainCount = 1,
            .pSwapchains = &*swapChain.vkSwapChain,
            .pImageIndices = &imageIndex
        };
        const vk::Result result = context.queue.presentKHR(presentInfo);
        if (result == vk::Result::eSuboptimalKHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChainResources(windowSize);
        }
        else if (result != vk::Result::eSuccess) {
            throw std::runtime_error("failed to present swap chain image");
        }
    }
    catch (const vk::SystemError& error) {
        if (error.code().value() ==
            static_cast<int>(vk::Result::eErrorOutOfDateKHR)) {
            recreateSwapChainResources(windowSize);
            return;
        }
        throw;
    }

    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

RenderApp::RenderApp(
    AppState& state,
    VulkanContext& context,
    WindowApp& windowApp,
    SwapChain& swapChain,
    ImguiApp& imgui
)
    : state(state),
      context(context),
      windowApp(windowApp),
      swapChain(swapChain),
      imgui(imgui) {
    init();
}

RenderApp::~RenderApp() = default;

void RenderApp::run() {
    windowApp.run();
}
