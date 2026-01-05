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
typedef std::unique_ptr<
    GLFWwindow,
    decltype([](GLFWwindow* window) {
        if (window) {
            glfwDestroyWindow(window);
        }
    })>
    GLFWwindowWrapper;

class WindowApp {
private:
    // unique_ptr makes WindowApp moveable but not copyable
    GLFWwindowWrapper window;
    static void resizeCallBackHelper(GLFWwindow* window, int width, int height);

public:
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
    Size2D<int> getWindowSize();
    Size2D<int> getFrameSize() const;
    bool isMinimized() const;
    vk::raii::SurfaceKHR createSurface(const vk::raii::Instance& instance);
    float getScale() const;
};

#endif  // WINDOWAPP_HPP
