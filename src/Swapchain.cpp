#include "Swapchain.hpp"

#include <print>
#include <vulkan/vulkan_raii.hpp>

#include "VulkanContext.hpp"

namespace {
vk::Extent2D chooseSwapExtent(
    const vk::SurfaceCapabilitiesKHR& capabilities,
    vk::Extent2D fbSize
) {
    if (capabilities.currentExtent.width != 0xFFFFFFFF) {
        return capabilities.currentExtent;
    }

    return {
        .width = std::clamp<uint32_t>(
            fbSize.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width
        ),
        .height = std::clamp<uint32_t>(
            fbSize.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height
        )
    };
}

vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR>& availableFormats
) {
    assert(!availableFormats.empty());
    for (const auto& format : availableFormats) {
        if ((format.format == vk::Format::eB8G8R8A8Srgb ||
             format.format == vk::Format::eR8G8B8A8Srgb) &&
            format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return format;
        }
    }
    for (const auto& format : availableFormats) {
        if ((format.format == vk::Format::eB8G8R8A8Unorm ||
             format.format == vk::Format::eR8G8B8A8Unorm) &&
            format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            std::println("Can not found Srgb Format, using Unorm.");
            return format;
        }
    }
    throw std::runtime_error("Format not supported yet");
    // return availableFormats[0];
}

uint32_t chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR& surfaceCapabilities) {
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    if ((0 < surfaceCapabilities.maxImageCount) &&
        (surfaceCapabilities.maxImageCount < minImageCount)) {
        minImageCount = surfaceCapabilities.maxImageCount;
    }
    return minImageCount;
}

vk::PresentModeKHR chooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR>& availablePresentModes
) {
    // for (const auto& presentMode : availablePresentModes) {
    //     if (presentMode == vk::PresentModeKHR::eMailbox) {
    //         return vk::PresentModeKHR::eMailbox;
    //     }
    // }
    return vk::PresentModeKHR::eFifo;
}
}  // namespace

void SwapChain::init(const VulkanContext& context, Size2D<uint32_t> size) {
    auto surfaceCapabilities =
        context.physicalDevice.getSurfaceCapabilitiesKHR(context.surface);
    this->extent = chooseSwapExtent(
        surfaceCapabilities,
        vk::Extent2D{size.width, size.height}
    );
    this->surfaceFormat = chooseSwapSurfaceFormat(
        context.physicalDevice.getSurfaceFormatsKHR(context.surface)
    );
    minImageCount = chooseSwapMinImageCount(
        context.physicalDevice.getSurfaceCapabilitiesKHR(context.surface)
    );
    vk::SwapchainCreateInfoKHR swapChainCreateInfo{
        .surface = context.surface,
        .minImageCount = minImageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = chooseSwapPresentMode(
            context.physicalDevice.getSurfacePresentModesKHR(context.surface)
        ),
        .clipped = true
    };

    this->swapChain = vk::raii::SwapchainKHR{context.device, swapChainCreateInfo};
    auto swapChainImages = swapChain.getImages();

    vk::ImageViewCreateInfo imageViewCreateInfo{
        .viewType = vk::ImageViewType::e2D,
        .format = surfaceFormat.format,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    for (auto& image : swapChainImages) {
        imageViewCreateInfo.image = image;
        this->images.emplace_back(
            std::move(image),
            vk::raii::ImageView{context.device, imageViewCreateInfo},
            vk::raii::Semaphore{context.device, vk::SemaphoreCreateInfo{}}
        );
    }
};

SwapChain::SwapChain(const VulkanContext& context, Size2D<uint32_t> size) {
    init(context, size);
}

void SwapChain::recreate(const VulkanContext& context, Size2D<uint32_t> size) {
    context.device.waitIdle();
    // Don't use release, which does not delete the resource.
    swapChain = nullptr;
    extent = vk::Extent2D{0, 0},
    images.clear();
    
    init(context, size);
};
