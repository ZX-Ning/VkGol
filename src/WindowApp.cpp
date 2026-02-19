#include "WindowApp.hpp"

// std c++
#include <cassert>
#include <functional>
#include <print>
#include <stdexcept>

// vulkan
#include <tuple>
#include <vulkan/vulkan_raii.hpp>

// glfw
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include "AppState.hpp"

void WindowApp::updateState(AppState& state) {
    GLFWwindow* window = this->window.get();
    state.inputs.mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    state.inputs.lastMousePos = state.inputs.mousePos;
    state.inputs.mousePos = {(float)x, (float)y};

    if (state.inputs.mouseState == GLFW_PRESS || !state.showImGui) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    if (state.quit) {
        glfwSetCursor(window, NULL);
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void WindowApp::resizeCallBackHelper(GLFWwindow* window, int width, int height) {
    auto windowPtr = glfwGetWindowUserPointer(window);
    assert(windowPtr != nullptr);
    WindowApp* self = reinterpret_cast<WindowApp*>(windowPtr);
    assert(window == self->window.get());

    self->getScale();
    try {
        self->resizeCallBack(width, height);
    }
    catch (const std::bad_function_call& e) {
    }
}

void WindowApp::keyCallbackHelper(GLFWwindow* window, int key, int, int action, int) {
    auto windowPtr = glfwGetWindowUserPointer(window);
    assert(windowPtr != nullptr);
    WindowApp* self = reinterpret_cast<WindowApp*>(windowPtr);
    assert(window == self->window.get());
    try {
        self->keyCallback(key, action);
    }
    catch (const std::bad_function_call& e) {
    }
}

WindowApp::WindowApp(
    int width, int height, std::string_view tittle, AppState& state
) : state(state) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_TRUE);

    window.reset(glfwCreateWindow(
        width,
        height,
        tittle.data(),
        nullptr,
        nullptr
    ));
    void* selfPtr = this;
    glfwSetWindowUserPointer((GLFWwindow*)window.get(), selfPtr);
    glfwSetFramebufferSizeCallback(window.get(), &resizeCallBackHelper);
    glfwSetKeyCallback(window.get(), &keyCallbackHelper);
    handCursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
    glfwSetInputMode(window.get(), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
}

WindowApp::~WindowApp() = default;

void WindowApp::run() {
    while (!glfwWindowShouldClose(window.get())) {
        glfwPollEvents();
        updateState(state);
        drawFrameCallBack();
    }
    cleanupCallBack();
}

Size2D<int> WindowApp::getWindowSize() const {
    int width, height;
    glfwGetWindowSize(window.get(), &width, &height);
    return {width, height};
};

Size2D<int> WindowApp::getFrameSize() const {
    int width, height;
    glfwGetFramebufferSize(window.get(), &width, &height);
    return {width, height};
}

GLFWwindow* WindowApp::getWindowPtr() {
    return window.get();
};

bool WindowApp::isMinimized() const {
    int iconified = glfwGetWindowAttrib(window.get(), GLFW_ICONIFIED);
    return iconified;
}

vk::raii::SurfaceKHR WindowApp::createSurface(const vk::raii::Instance& instance) {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(
            *instance, window.get(), nullptr, &surface
        ) != 0) {
        throw std::runtime_error("failed to create window surface!");
    }
    return {instance, surface};
}

std::tuple<WindowApp::ScalingType, float> WindowApp::getScale() {
    auto winSize = getWindowSize();
    auto fbSize = getFrameSize();
    scalingType = fbSize.height == winSize.height ? WINDOWS_OR_X11 : MAC_OR_WAYLAND;
    float xscale, yscale;
    glfwGetWindowContentScale(window.get(), &xscale, &yscale);
    float scale = xscale / 2 + yscale / 2;

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
    return glfwGetKey(window.get(), key);
}
