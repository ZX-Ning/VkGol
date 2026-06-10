// imgui
#ifndef IMGUI_HPP
#define IMGUI_HPP

#include <imgui.h>

#include <cstdint>
#include <vulkan/vulkan.hpp>

#include "utils.hpp"

struct WindowApp;
struct AppState;
struct VulkanContext;
struct Scene;
struct SwapChain;
struct SDL_Window;
namespace vk::raii {
class CommandBuffer;
}

struct ImguiApp {
    explicit ImguiApp(WindowApp& window, VulkanContext& context, SwapChain& swapChain);
    void initForSdl(WindowApp& windowApp);
    static constexpr int FPS_UPDATE_INTERVAL_MS = 200;
    uint64_t lastShowFpsTime;
    float lastframeTime;
    void initImguiForVk(
        WindowApp& windowApp,
        const VulkanContext& context,
        vk::Format format,
        uint32_t imageCount
    );
    ImDrawData* drawImgui(AppState& state);
    bool wantMouse();
    void renderVk(ImDrawData* drawData, vk::raii::CommandBuffer& cmd);
    ~ImguiApp();
    DISABLE_COPY(ImguiApp)
};

#endif  // IMGUI_HPP
