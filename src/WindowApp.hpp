#ifndef WINDOWAPP_HPP
#define WINDOWAPP_HPP

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>

#include <functional>
#include <memory>
#include <string_view>

#include "utils.hpp"

struct AppState;
namespace vk::raii {
class SurfaceKHR;
class Instance;
}  // namespace vk::raii

struct WindowDeleter {
    void operator()(SDL_Window* window) const noexcept;
};
using SDLWindowWrapper = std::unique_ptr<SDL_Window, WindowDeleter>;

struct WindowApp {
private:
    SDLWindowWrapper window;
    bool shouldClose = false;
    void processEvent(const SDL_Event& event);
    void updateState(AppState& state);

public:
    enum ScalingType {
        MAC_OR_WAYLAND,
        WINDOWS_OR_X11,
    } scalingType;
    explicit WindowApp(int width, int height, std::string_view title, AppState& state);

    WindowApp(const WindowApp&) = delete;
    WindowApp& operator=(const WindowApp&) = delete;
    WindowApp(WindowApp&&) = delete;
    WindowApp& operator=(WindowApp&&) = delete;
    ~WindowApp();

    std::function<void(int width, int height)> resizeCallBack;
    std::function<void()> drawFrameCallBack;
    std::function<void()> cleanupCallBack;
    std::function<void(int key, int action)> keyCallback;
    std::function<void(const SDL_Event& event)> eventCallback;
    AppState& state;

    void run();
    Size2D<int> getWindowSize() const;
    Size2D<int> getFrameSize() const;
    SDL_Window* getWindowPtr();
    bool isMinimized() const;
    vk::raii::SurfaceKHR createSurface(const vk::raii::Instance& instance);
    std::tuple<ScalingType, float> getScale();
    int getKeyState(int key);
};

#endif  // WINDOWAPP_HPP
