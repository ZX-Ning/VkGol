#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP

#include <vulkan/vulkan_raii.hpp>

#include "../utils.hpp"

class VulkanContext;

struct SurfaceImages {
    vk::Image image;
    vk::raii::ImageView imageView{nullptr};
    // vk::raii::ImageView imageViewNorm;
    vk::raii::Semaphore renderComplete{nullptr};
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
