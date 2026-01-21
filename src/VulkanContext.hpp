#ifndef VULKANCONTEXT_HPP
#define VULKANCONTEXT_HPP

#include <vulkan/vulkan_raii.hpp>

#include "VulkanUtils.hpp"

class WindowApp;

struct VulkanContext {
    vk::raii::Context context;
    vk::raii::Instance instance{nullptr};
    vk::raii::DebugUtilsMessengerEXT debugMessenger{nullptr};
    vk::raii::PhysicalDevice physicalDevice{nullptr};
    vk::raii::Device device{nullptr};
    vk::raii::SurfaceKHR surface{nullptr};
    vk::raii::Queue queue{nullptr};
    vk::raii::CommandPool commandPool{nullptr};
    uint32_t queueFamilyIndex = ~0;
    vk::raii::CommandBuffer loadingCmdBuffer{nullptr};
    VmaAllocatorWrapper allocator;
    void initLogicalDevice();
    void initVmaAllocator();
    VulkanContext(WindowApp& windowApp);
    static vk::raii::detail::InstanceDispatcher* getStaticInstanceDispatcher();
};

#endif  // VULKANCONTEXT_HPP
