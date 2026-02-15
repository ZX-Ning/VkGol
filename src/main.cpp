#include <memory>
#include <print>

#include "AppState.hpp"
#include "Consts.hpp"
#include "ModelLoader.hpp"
#include "RenderApp.hpp"
#include "Scene.hpp"
#include "WindowApp.hpp"
#include "core/Swapchain.hpp"
#include "core/Texture.hpp"
#include "core/VulkanContext.hpp"

struct DiManager {
    std::unique_ptr<WindowApp> window;
    std::unique_ptr<VulkanContext> context;
    std::unique_ptr<SwapChain> swapchain;
    Model defaultModel;
    std::unique_ptr<RenderObject> defaultRenderobj;
    std::unique_ptr<Scene> scene;
    std::unique_ptr<AppState> state;
    std::unique_ptr<RenderApp> renderApp;

    DiManager() {
        window = std::make_unique<WindowApp>(Consts::WIDTH, Consts::HEIGHT, Consts::TITTLE);
        context = std::make_unique<VulkanContext>(*window);
        swapchain = std::make_unique<SwapChain>(*context, window->getFrameSize());
        defaultModel = ModelLoader::loadSimpleTraingleModel(*context);
        defaultRenderobj.reset(new RenderObject{
            .model = defaultModel,
            .position = {0.f, 0.f, 1.f},
            .scale = {1.f, 1.f, 1.f},
            .angle = 0.f,
            .axis = {0.f, 1.f, 0.f}
        });
        scene.reset(new Scene{
            .objects = {*defaultRenderobj},
            .view = {.eye = {0.f, 3.f, -8.f}, .target = {0.f, 1.f, 0.f}, .up = {0.f, 0.1f, 0.f}},
            .camera = {.fovy = degreeToRadian(37.8), .aspectRatio = 1.f, .clipNear = 0.1f, .clipFar = 200.f},
        });
        state = std::make_unique<AppState>();
        renderApp = std::make_unique<RenderApp>(*context, *window, *swapchain);
    }
};

int main() {
    try {
        DiManager manager;
        manager.renderApp->setScene(*manager.scene);
        manager.renderApp->run();
        // app.run();
    }
    catch (const std::exception& e) {
        std::println(stderr, "Error: {}", e.what());
        return 1;
    }
    return 0;
}
