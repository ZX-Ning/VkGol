#ifndef RENDER_HPP
#define RENDER_HPP

// c++ std libs
#include <cstdint>
#include <memory>
#include <vector>

// vulkan-hpp headers
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_structs.hpp>

// glfw
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

// project
#include "AppState.hpp"
#include "core/Buffer.hpp"
#include "core/Model.hpp"
#include "utils.hpp"

// forward declaration
struct WindowApp;
struct VulkanContext;
struct SwapChain;
struct Scene;
////////////////////////

struct FrameData {
    vk::raii::CommandBuffer cmdBuffer{nullptr};
    vk::raii::Semaphore presentComplete{nullptr};
    vk::raii::Fence fences{nullptr};
    vk::raii::DescriptorSet sceneDescriptorSet{nullptr};
    std::shared_ptr<DynamicBuffer> sceneUniformBuf;
};

struct RenderApp {
private:
    VulkanContext& context;
    WindowApp& windowApp;
    SwapChain& swapChain;
    OptRef<Scene> scene;
    AppState state;
    std::vector<FrameData> frames;
    uint32_t frameIndex = 0;

    bool framebufferResized = false;
    void initFrames();

public:
    RenderApp(VulkanContext& context, WindowApp& windowApp, SwapChain& swapChain);
    ~RenderApp();
    void init();
    void run();
    void drawFrame();
    void setScene(Scene& scene);
    DISABLE_COPY(RenderApp)
};

#endif // RENDER_HPP
