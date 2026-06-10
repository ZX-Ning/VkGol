#ifndef FORWARDPASS_HPP
#define FORWARDPASS_HPP

#include <vulkan/vulkan.hpp>

#include "../utils.hpp"

struct AppState;
struct FrameContext;
struct ImguiApp;
struct SurfaceImage;
struct VulkanContext;

struct ForwardPassContext {
    VulkanContext& context;
    FrameContext& frame;
    SurfaceImage& target;
    vk::Extent2D extent;
    Size2D<int> windowSize;
    AppState& state;
    ImguiApp& imgui;
};

struct ForwardPass {
    void record(const ForwardPassContext& ctx);
};

#endif  // FORWARDPASS_HPP
