#include "App.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Consts.hpp"
#include "ModelLoader.hpp"

App::App() {
    state = std::make_unique<AppState>();
    window = std::make_unique<WindowApp>(Consts::WIDTH, Consts::HEIGHT, Consts::TITTLE, *state);
    context = std::make_unique<VulkanContext>(*window);
    swapchain = std::make_unique<SwapChain>(*context, window->getFrameSize());
    imgui = std::make_unique<ImguiApp>(*window, *context, *swapchain);
    defaultModel = ModelLoader::loadSimpleTraingleModel(*context);
    defaultRenderobj.reset(new RenderObject{
        .model = defaultModel,
        .position = {0.f, 0.f, 1.f},
        .scale = {1.f, 1.f, 1.f},
        .angle = 0.f,
        .axis = {0.f, 1.f, 0.f}
    });
    std::vector<Ref<RenderObject>> objs{*defaultRenderobj};
    scene = std::make_unique<Scene>(objs, *state);
    renderApp = std::make_unique<RenderApp>(
        *state,
        *context,
        *window,
        *swapchain,
        *imgui
    );
}

void App::setupCallback() {
    (*window).cleanupCallBack =
        [this]() {
            this->context->device.waitIdle();
        };
    (*window).resizeCallBack =
        [this](int, int) {
            this->renderApp->framebufferResized = true;
        };
    (*window).drawFrameCallBack = std::bind(&App::onDraw, this);
    (*window).keyCallback = ([this](int key, int action) {
        switch (key) {
            case GLFW_KEY_ESCAPE: {
                if (action == GLFW_PRESS) {
                    this->state->showImGui = !this->state->showImGui;
                }
                break;
            }
            default: {
                break;
            }
        }
    });
}

void App::onDraw() {
    if (state->rotationSpeed > 1.f) {
        auto& obj = *defaultRenderobj;
        float dAngle = degreeToRadian(state->rotationSpeed / 1000 * state->frameTime);
        float angle = obj.angle + dAngle;
        obj.angle = std::fmod(angle, std::numbers::pi * 2);
    }
    this->renderApp->drawFrame();
}

void App::run() {
    setupCallback();
    renderApp->setScene(*scene);
    renderApp->run();
}
