#include <exception>
#include <print>

#include "AppState.hpp"
#include "Consts.hpp"
#include "ImguiApp.hpp"
#include "WindowApp.hpp"
#include "core/Swapchain.hpp"
#include "core/VulkanContext.hpp"
#include "renderer/RenderApp.hpp"

int main() {
    try {
        AppState state;
        WindowApp window{
            Consts::WIDTH,
            Consts::HEIGHT,
            Consts::TITLE,
            state
        };
        VulkanContext context{window};
        SwapChain swapChain{context, window.getFrameSize()};
        ImguiApp imgui{window, context, swapChain};
        RenderApp renderer{
            state,
            context,
            window,
            swapChain,
            imgui
        };

        window.cleanupCallBack = [&context]() {
            context.device.waitIdle();
        };
        window.resizeCallBack = [&renderer](int, int) {
            renderer.framebufferResized = true;
        };
        window.drawFrameCallBack = [&renderer]() {
            renderer.drawFrame();
        };
        renderer.run();
    }
    catch (const std::exception& error) {
        std::println(stderr, "Error: {}", error.what());
        return 1;
    }

    std::println("Done.");
    return 0;
}
