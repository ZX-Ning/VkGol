#ifndef APP_HPP
#define APP_HPP

#include <memory>

#include "AppState.hpp"
#include "RenderApp.hpp"
#include "Scene.hpp"
#include "WindowApp.hpp"
#include "core/Model.hpp"
#include "core/Swapchain.hpp"
#include "core/Texture.hpp"
#include "core/VulkanContext.hpp"

struct App {
private:
    std::unique_ptr<AppState> state;
    std::unique_ptr<WindowApp> window;
    std::unique_ptr<VulkanContext> context;
    std::unique_ptr<SwapChain> swapchain;
    std::unique_ptr<ImguiApp> imgui;
    Model defaultModel;
    std::unique_ptr<RenderObject> defaultRenderobj;
    std::unique_ptr<Scene> scene;
    std::unique_ptr<RenderApp> renderApp;

    void setupCallback();
    void onDraw();

public:
    App();
    void run();
};

#endif  // APP_HPP
