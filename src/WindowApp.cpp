#include "WindowApp.hpp"

// std c++
#include <cassert>
#include <functional>
#include <print>
#include <stdexcept>

// vulkan
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
    std::println("Resized: {}x{}", width, height);
    self->resizeCallBack(width, height);
}

WindowApp::WindowApp(int width, int height, std::string_view tittle) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
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

    auto fbSize = getFrameSize();
    if (fbSize.height == height) {
        scalingType = WINDOWS_OR_X11;
        float scale = getScale();
        width *= scale;
        height *= scale;
        fbSize.height *= scale;
        fbSize.width *= scale;
        glfwSetWindowSize(window.get(), width, height);
    }
    else {
        scalingType = MAC_OR_WAYLAND;
    }

    std::println(
        "Created a GLFW window; Size:{} x {}; Framebuffer Size: {}x{}, Scale method: {}",
        width,
        height,
        fbSize.width,
        fbSize.height,
        scalingType == WINDOWS_OR_X11 ? "win/x11" : "mac/wayland"
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

float WindowApp::getScale() const {
    float xscale, yscale;
    glfwGetWindowContentScale(window.get(), &xscale, &yscale);
    return xscale / 2 + yscale / 2;
};
