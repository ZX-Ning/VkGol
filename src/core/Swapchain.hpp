#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP

#include <vulkan/vulkan_raii.hpp>

#include "../utils.hpp"

struct VulkanContext;
struct Texture;

struct SurfaceImage {
    vk::Image image;
    vk::raii::ImageView imageView{nullptr};
    vk::raii::Semaphore imageAcquired{nullptr};
    std::unique_ptr<Texture> depthTexture;
};

struct SwapChain {
private:
    void init(const VulkanContext& context, Size2D<uint32_t> size);

public:
    struct SwapChainOutOfDateError : public std::runtime_error {
        SwapChainOutOfDateError() : std::runtime_error("SwapChain OutOfDate!") {}
    };
    vk::raii::SwapchainKHR vkSwapChain{nullptr};
    vk::SurfaceFormatKHR surfaceFormat;
    vk::Extent2D extent;
    std::vector<SurfaceImage> images;
    uint32_t minImageCount;
    explicit SwapChain(const VulkanContext& context, Size2D<uint32_t> size);
    void recreate(const VulkanContext& context, Size2D<uint32_t> size);
    uint32_t acquireNextImage(vk::raii::Semaphore& semaphore);
};

#endif  // SWAPCHAIN_HPP
