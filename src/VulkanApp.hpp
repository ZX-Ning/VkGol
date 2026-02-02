#ifndef VULKANAPP_HPP
#define VULKANAPP_HPP

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
#include "Camera.hpp"
#include "core/Buffer.hpp"
#include "core/Model.hpp"
#include "core/UniformData.hpp"
#include "utils.hpp"
#include "Object.hpp"

// forward declaration
struct WindowApp;
struct VulkanContext;
struct SwapChain;
////////////////////////
struct VulkanApp {
public:
    struct FrameData {
        vk::raii::CommandBuffer cmdBuffer{nullptr};
        vk::raii::Semaphore presentComplete{nullptr};
        vk::raii::Fence fences{nullptr};
        vk::raii::DescriptorSet sceneDescriptorSet{nullptr};
        std::shared_ptr<DynamicBuffer> sceneUniformBuf;
    };
    struct AppState {
        RGBAColor clearColor{0.45f, 0.53f, 0.65f, 1.f};
        View view;
        Camera camera;
        std::unique_ptr<Object> obj;
        bool showDemoWindow = false;
        uint64_t lastRenderTimestamp;
        float frameTime;
    };

private:
    std::unique_ptr<VulkanContext> context;
    std::unique_ptr<SwapChain> swapChain;
    std::unique_ptr<WindowApp> windowApp;
    std::vector<FrameData> frames;

    Model model;

    DefaultScenceUBO ubo;

    // vk::raii::Pipeline graphicsPipeline{nullptr};
    // std::shared_ptr<StaticBuffer> vertexBuffer;
    // uint32_t minImageCount;
    uint32_t frameIndex = 0;
    AppState state;

    bool framebufferResized = false;
    void init();
    void initImgui();
    void initFrames();
    void drawFrame();

public:
    explicit VulkanApp(std::unique_ptr<WindowApp>&& window);
    ~VulkanApp();
    void run();
    DISABLE_COPY(VulkanApp)
};

#endif  // VULKANAPP_HPP
