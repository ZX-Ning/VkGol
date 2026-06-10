#include "WindowApp.hpp"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_vulkan.h>

#include <format>
#include <functional>
#include <print>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vulkan/vulkan_raii.hpp>

#include "AppState.hpp"

void WindowDeleter::operator()(SDL_Window* window) const noexcept {
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

void WindowApp::processEvent(const SDL_Event& event) {
    try {
        eventCallback(event);
    }
    catch (const std::bad_function_call&) {
    }

    switch (event.type) {
        case SDL_EVENT_QUIT: {
            shouldClose = true;
            break;
        }
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
            if (event.window.windowID == SDL_GetWindowID(window.get())) {
                shouldClose = true;
            }
            break;
        }
        case SDL_EVENT_DISPLAY_CONTENT_SCALE_CHANGED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        case SDL_EVENT_WINDOW_RESIZED: {
            getScale();
            auto size = getFrameSize();
            try {
                resizeCallBack(size.width, size.height);
            }
            catch (const std::bad_function_call&) {
            }
            break;
        }
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP: {
            if (event.key.windowID == SDL_GetWindowID(window.get())) {
                try {
                    keyCallback(event.key.key, event.key.down ? 1 : 0);
                }
                catch (const std::bad_function_call&) {
                }
            }
            break;
        }
        default: {
            break;
        }
    }
}

void WindowApp::updateState(AppState& state) {
    const bool grabMouse =
        state.inputs.mouseState || !state.showImGui;
    if (SDL_GetWindowRelativeMouseMode(window.get()) != grabMouse) {
        SDL_SetWindowRelativeMouseMode(window.get(), grabMouse);
    }

    float x = 0.f;
    float y = 0.f;
    const SDL_MouseButtonFlags buttons = grabMouse
        ? SDL_GetRelativeMouseState(&x, &y)
        : SDL_GetMouseState(&x, &y);

    state.inputs.mouseState =
        (buttons & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) ? 1 : 0;
    state.inputs.lastMousePos = state.inputs.mousePos;
    if (grabMouse) {
        state.inputs.mousePos += glm::fvec2{x, y};
    }
    else {
        state.inputs.mousePos = {x, y};
    }

    if (state.quit) {
        shouldClose = true;
        SDL_SetCursor(nullptr);
    }
}

WindowApp::WindowApp(
    int width, int height, std::string_view title, AppState& state
) : state(state) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        throw std::runtime_error(
            std::format("failed to init SDL: {}", SDL_GetError())
        );
    }

    window.reset(SDL_CreateWindow(
        std::string(title).c_str(),
        width,
        height,
        SDL_WINDOW_VULKAN |
            SDL_WINDOW_RESIZABLE |
            SDL_WINDOW_HIGH_PIXEL_DENSITY
    ));
    if (!window) {
        std::string error = SDL_GetError();
        SDL_Quit();
        throw std::runtime_error(
            std::format("failed to create SDL window: {}", error)
        );
    }
}

WindowApp::~WindowApp() = default;

void WindowApp::run() {
    while (!shouldClose) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            processEvent(event);
        }
        updateState(state);
        drawFrameCallBack();
    }
    cleanupCallBack();
}

Size2D<int> WindowApp::getWindowSize() const {
    int width = 0;
    int height = 0;
    SDL_GetWindowSize(window.get(), &width, &height);
    return {width, height};
};

Size2D<int> WindowApp::getFrameSize() const {
    int width = 0;
    int height = 0;
    SDL_GetWindowSizeInPixels(window.get(), &width, &height);
    return {width, height};
}

SDL_Window* WindowApp::getWindowPtr() {
    return window.get();
};

bool WindowApp::isMinimized() const {
    return (SDL_GetWindowFlags(window.get()) & SDL_WINDOW_MINIMIZED) != 0;
}

vk::raii::SurfaceKHR WindowApp::createSurface(const vk::raii::Instance& instance) {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (!SDL_Vulkan_CreateSurface(window.get(), *instance, nullptr, &surface)) {
        throw std::runtime_error(
            std::format("failed to create window surface: {}", SDL_GetError())
        );
    }
    return {instance, surface};
}

std::tuple<WindowApp::ScalingType, float> WindowApp::getScale() {
    auto winSize = getWindowSize();
    auto fbSize = getFrameSize();
    scalingType = fbSize.height == winSize.height ? WINDOWS_OR_X11 : MAC_OR_WAYLAND;
    float scale = SDL_GetWindowDisplayScale(window.get());

    std::println(
        "[Window] Size:{} x {}; Framebuffer Size: {}x{}; scale: {}; Scale method: {}.",
        winSize.width,
        winSize.height,
        fbSize.width,
        fbSize.height,
        scale,
        scalingType == WINDOWS_OR_X11 ? "win/x11" : "mac/wayland"
    );

    return {scalingType, scale};
};

int WindowApp::getKeyState(int key) {
    int numKeys = 0;
    const bool* keys = SDL_GetKeyboardState(&numKeys);
    if (key < 0 || key >= numKeys) {
        return 0;
    }
    return keys[key] ? 1 : 0;
}
