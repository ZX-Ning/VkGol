#ifndef RENDER_HPP
#define RENDER_HPP

#include <cstdint>
#include <memory>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "../utils.hpp"
#include "FrameContext.hpp"

struct AppState;
struct GOLPass;
struct ImguiApp;
struct Pipeline;
struct StaticBuffer;
struct SurfaceImage;
struct SwapChain;
struct VulkanContext;
struct WindowApp;

struct RenderApp {
private:
    AppState& state;
    VulkanContext& context;
    WindowApp& windowApp;
    SwapChain& swapChain;
    ImguiApp& imgui;

    std::unique_ptr<GOLPass> golPass;
    vk::raii::DescriptorSetLayout textureSetLayout{nullptr};
    vk::raii::PipelineLayout pipelineLayout{nullptr};
    vk::raii::DescriptorSet textureSet{nullptr};
    std::shared_ptr<Pipeline> renderPipeline;
    std::shared_ptr<StaticBuffer> quadVertexBuffer;

    std::vector<FrameContext> frames;
    uint32_t frameIndex = 0;

    void init();
    void recreateSwapChainResources(Size2D<uint32_t> size);
    void record(
        vk::raii::CommandBuffer& cmd,
        SurfaceImage& target,
        vk::Extent2D extent
    );

public:
    bool framebufferResized = false;

    RenderApp(
        AppState& state,
        VulkanContext& context,
        WindowApp& windowApp,
        SwapChain& swapChain,
        ImguiApp& imgui
    );
    ~RenderApp();

    void run();
    void drawFrame();

    DISABLE_COPY(RenderApp)
};

#endif  // RENDER_HPP
