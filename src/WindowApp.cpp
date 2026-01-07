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

// imgui
#include <backends/imgui_impl_glfw.h>

void WindowApp::resizeCallBackHelper(GLFWwindow* window, int width, int height) {
    auto windowPtr = glfwGetWindowUserPointer(window);
    assert(windowPtr != nullptr);
    WindowApp* self = reinterpret_cast<WindowApp*>(windowPtr);
    assert(window == self->window.get());
    self->getScale();
    self->resizeCallBack(width, height);
}

WindowApp::WindowApp(int width, int height, std::string_view tittle) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_TRUE);

    window.reset(glfwCreateWindow(
        width,
        height,
        "Learn Vulkan",
        nullptr,
        nullptr
    ));
    void* selfPtr = this;
    glfwSetWindowUserPointer((GLFWwindow*)window.get(), selfPtr);
    glfwSetFramebufferSizeCallback(
        window.get(),
        &resizeCallBackHelper
    );

    ImGui::CreateContext();
    if (!ImGui_ImplGlfw_InitForVulkan(window.get(), true)) {
        throw std::runtime_error("Failed to init IMGUI for GLFW");
    }
}

void WindowApp::run() {
    while (!glfwWindowShouldClose(window.get())) {
        glfwPollEvents();
        drawFrameCallBack();
    }
    cleanupCallBack();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfwTerminate();
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
