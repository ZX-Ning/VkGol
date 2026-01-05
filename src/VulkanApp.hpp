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

// VMA
#include <vk_mem_alloc.h>

// glfw
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

// project
#include "WindowApp.hpp"
#include "utils.hpp"
#include "VulkanUtils.hpp"
#include "VmaBuffer.hpp"

constexpr int MAX_FRAMES_IN_FLIGHT = 2;
constexpr bool ENABLE_VALIDATION_LAYERS = IS_DEBUG;
inline const std::vector<char const*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};
inline const std::vector<const char*> requiredDeviceExtension = {
    vk::KHRSwapchainExtensionName,
    vk::KHRSpirv14ExtensionName,
    vk::KHRSynchronization2ExtensionName,
    vk::KHRCreateRenderpass2ExtensionName,
    vk::KHRDynamicRenderingExtensionName,
    vk::EXTMemoryBudgetExtensionName
};

// forward declaration
class WindowApp;
////////////////////////


class VulkanApp {
public:
    struct SurfaceImages {
        vk::Image image;
        vk::raii::ImageView imageView{nullptr};
        // vk::raii::ImageView imageViewNorm;
        vk::raii::Semaphore renderComplete{nullptr};
    };
    struct SwapChain {
        vk::raii::SwapchainKHR swapChain{nullptr};
        vk::SurfaceFormatKHR surfaceFormat;
        vk::Extent2D extent;
        std::vector<SurfaceImages> images;
        void reset() {
            swapChain.release();
            extent = {0, 0},
            images.clear();
        }
    };
    // struct SimpleBuffer {
    //     vk::raii::Buffer buffer{nullptr};
    //     vk::raii::DeviceMemory memory{nullptr};
    // };
    struct Frame {
        vk::raii::CommandBuffer cmdBuffer{nullptr};
        vk::raii::Semaphore presentComplete{nullptr};
        vk::raii::Fence fences{nullptr};
    };
    struct AppState {
        RGBAColor clearColor{0.45f, 0.53f, 0.65f, 1.f};
        bool showDemoWindow = false;
        uint64_t lastRenderTimestamp;
        float frameTime;
    };

private:
    std::unique_ptr<WindowApp> windowApp;
    vk::raii::Context context;
    vk::raii::Instance instance{nullptr};
    vk::raii::DebugUtilsMessengerEXT debugMessenger{nullptr};
    vk::raii::SurfaceKHR surface{nullptr};
    vk::raii::PhysicalDevice physicalDevice{nullptr};
    vk::raii::Device device{nullptr};
    vk::raii::Queue queue{nullptr};
    SwapChain swapChain;
    VmaAllocatorWrapper allocator;
    vk::raii::Pipeline graphicsPipeline{nullptr};
    std::unique_ptr<VmaBuffer> vertexBuffer;
    vk::raii::CommandPool commandPool{nullptr};
    std::vector<Frame> frames;
    uint32_t minImageCount;
    uint32_t queueFamilyIndex = ~0;
    uint32_t frameIndex = 0;
    AppState state;

    bool framebufferResized = false;
    void init();
    void initImgui();
    void recreateSwapChain();
    void drawFrame();

public:
    explicit VulkanApp(std::unique_ptr<WindowApp>&& window);
    void run();

    DISABLE_COPY(VulkanApp)
};

#endif  // VULKANAPP_HPP
