#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP

#include <vulkan/vulkan_raii.hpp>

#include "../utils.hpp"

struct VulkanContext;
struct Texture;

struct SurfaceImages {
    vk::Image image;
    vk::raii::ImageView imageView{nullptr};
    vk::raii::Semaphore renderComplete{nullptr};
    std::unique_ptr<Texture> depthTexture;
};

struct SwapChain {
private:
    void init(const VulkanContext& context, Size2D<uint32_t> size);

public:
    vk::raii::SwapchainKHR swapChain{nullptr};
    vk::SurfaceFormatKHR surfaceFormat;
    vk::Extent2D extent;
    std::vector<SurfaceImages> images;
    uint32_t minImageCount;
    explicit SwapChain(const VulkanContext& context, Size2D<uint32_t> size);
    void recreate(const VulkanContext& context, Size2D<uint32_t> size);
};

#endif  // SWAPCHAIN_HPP
