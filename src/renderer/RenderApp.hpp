#ifndef RENDER_HPP
#define RENDER_HPP

// c++ std libs
#include <cstdint>
#include <memory>
#include <vector>

// vulkan-hpp headers
#include <vulkan/vulkan_core.h>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_structs.hpp>

// project
#include "../AppState.hpp"
#include "../ImguiApp.hpp"
#include "../core/Buffer.hpp"
#include "../core/FrameContext.hpp"
#include "../core/Model.hpp"
#include "../utils.hpp"
#include "ForwardPass.hpp"

// forward declaration
struct WindowApp;
struct VulkanContext;
struct SwapChain;
struct Scene;
////////////////////////

struct RenderApp {
private:
    AppState& state;
    VulkanContext& context;
    WindowApp& windowApp;
    SwapChain& swapChain;
    ImguiApp& imgui;
    ForwardPass forwardPass;
    std::vector<FrameContext> frames;
    uint32_t frameIndex = 0;

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
    void init();
    void run();
    void drawFrame();
    DISABLE_COPY(RenderApp)
};

#endif  // RENDER_HPP
