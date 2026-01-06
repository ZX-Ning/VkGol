#ifndef WINDOWAPP_HPP
#define WINDOWAPP_HPP

#include <GLFW/glfw3.h>

#include <functional>
#include <string_view>

#include "utils.hpp"

// forward declarations
class GLFWwindow;
namespace vk::raii {
class SurfaceKHR;
class Instance;
}  // namespace vk::raii
///////////////////////////

// RAII wrapper for GLFWwindow
struct WindowDeleter {
    void operator()(GLFWwindow* window) const noexcept {
        if (window) {
            glfwDestroyWindow(window);
        }
    }
};
using GLFWwindowWrapper = std::unique_ptr<GLFWwindow, WindowDeleter>;

class WindowApp {
private:
    // unique_ptr makes WindowApp moveable but not copyable
    GLFWwindowWrapper window;
    static void resizeCallBackHelper(GLFWwindow* window, int width, int height);

public:
    enum ScalingType {
        MAC_OR_WAYLAND,
        WINDOWS_OR_X11,
    } scalingType;
    explicit WindowApp(int width, int height, std::string_view tittle);

    WindowApp(const WindowApp&) = delete;
    WindowApp& operator=(const WindowApp&) = delete;
    WindowApp(WindowApp&&) = delete;
    WindowApp& operator=(WindowApp&&) = delete;
    ~WindowApp() = default;

    std::function<void(int width, int height)> resizeCallBack;
    std::function<void()> drawFrameCallBack;
    std::function<void()> cleanupCallBack;
    void run();
    Size2D<int> getWindowSize() const;
    Size2D<int> getFrameSize() const;
    bool isMinimized() const;
    vk::raii::SurfaceKHR createSurface(const vk::raii::Instance& instance);
    float getScale() const;
};

#endif  // WINDOWAPP_HPP
