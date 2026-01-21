#include "VulkanContext.hpp"

#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>

#include <print>
#include <vulkan/vulkan_raii.hpp>

#include "VulkanApp.hpp"
#include "WindowApp.hpp"
#include "utils.hpp"

namespace {

constexpr bool ENABLE_VALIDATION_LAYERS = !IS_RELEASE;
const std::vector<char const*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};
const std::vector<const char*> requiredDeviceExtension = {
    vk::KHRSwapchainExtensionName,
    vk::KHRSpirv14ExtensionName,
    vk::KHRSynchronization2ExtensionName,
    vk::KHRCreateRenderpass2ExtensionName,
    vk::KHRDynamicRenderingExtensionName,
    vk::EXTMemoryBudgetExtensionName
};

// Static pointer to dispatchers to make Imgui and VMA work,
// Ugly but can not found other solutions.
// Require only one VulkanApp at a time.
static vk::raii::detail::InstanceDispatcher* instanceDispatcher;
static vk::raii::detail::DeviceDispatcher* deviceDispatcher;

std::vector<const char*> getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions =
        glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (ENABLE_VALIDATION_LAYERS) {
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }
    return extensions;
}

vk::raii::Instance createInstance(const vk::raii::Context& context) {
    constexpr vk::ApplicationInfo appInfo = {
        .pApplicationName = "Learn Vulkan",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vk::ApiVersion13
    };

    // Get the required layers
    std::vector<char const*> requiredLayers;
    if (ENABLE_VALIDATION_LAYERS) {
        std::println("Enable validation layers.");
        requiredLayers.assign(validationLayers.begin(), validationLayers.end());
    }
    else {
        std::println("Release mode. Disable validation layers.");
    }

    // Check if the required layers are supported by the Vulkan implementation.
    auto layerProperties = context.enumerateInstanceLayerProperties();
    for (auto const& requiredLayer : requiredLayers) {
        bool layerFound = false;
        for (auto const& layerProperty : layerProperties) {
            if (strcmp(layerProperty.layerName, requiredLayer) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            throw std::runtime_error(
                std::format("Required layer not supported: {}", std::string(requiredLayer))
            );
        }
    }

    // Get the required extensions.
    auto requiredExtensions = getRequiredExtensions();

    // Check if the required extensions are supported by the Vulkan
    // implementation.
    auto extensionProperties =
        context.enumerateInstanceExtensionProperties();
    for (auto const& requiredExtension : requiredExtensions) {
        bool extensionFound = false;
        for (auto const& extensionProperty : extensionProperties) {
            if (strcmp(extensionProperty.extensionName, requiredExtension) == 0) {
                extensionFound = true;
                break;
            }
        }

        int width, height;
        if (!extensionFound) {
            throw std::runtime_error(
                std::format("Required extension not supported: {}", requiredExtension)
            );
        }
    }

    vk::InstanceCreateInfo createInfo{
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames = requiredLayers.data(),
        .enabledExtensionCount =
            static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data()
    };
    return {context, createInfo};
}

vk::raii::DebugUtilsMessengerEXT setupDebugMessenger(const vk::raii::Instance& instance) {
    if (!ENABLE_VALIDATION_LAYERS) {
        return nullptr;
    }
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
    );
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
    );
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
        .messageSeverity = severityFlags,
        .messageType = messageTypeFlags,
        .pfnUserCallback =
            [](
                vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                vk::DebugUtilsMessageTypeFlagsEXT type,
                const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void*
            ) -> vk::Bool32 {
            if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError ||
                severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
                std::println(
                    "[{}] {}, {}",
                    vk::to_string(severity),
                    vk::to_string(type),
                    pCallbackData->pMessage
                );
            }
            return vk::False;
        }
    };
    return instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
}

vk::raii::PhysicalDevice pickPhysicalDevice(vk::raii::Instance& instance) {
    std::vector<vk::raii::PhysicalDevice> devices =
        instance.enumeratePhysicalDevices();

    // vk::raii::PhysicalDevice* selectedDevice = nullptr;

    for (vk::raii::PhysicalDevice& device : devices) {
        // Check if the device supports the Vulkan 1.3 API version
        bool supportsVulkan1_3 =
            device.getProperties().apiVersion >= VK_API_VERSION_1_3;

        // Check if any of the queue families support graphics operations
        auto queueFamilies = device.getQueueFamilyProperties();
        bool supportsGraphics = false;
        for (const vk::QueueFamilyProperties& qfp : queueFamilies) {
            if (qfp.queueFlags & vk::QueueFlagBits::eGraphics) {
                supportsGraphics = true;
                break;
            }
        }

        // Check if all required device extensions are available
        std::vector<vk::ExtensionProperties> availableDeviceExtensions =
            device.enumerateDeviceExtensionProperties();
        bool supportsAllRequiredExtensions = true;
        for (auto const& requiredDeviceExtension : requiredDeviceExtension) {
            bool extensionFound = false;
            for (auto const& availableDeviceExtension : availableDeviceExtensions) {
                if (strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0) {
                    extensionFound = true;
                    break;
                }
            }
            if (!extensionFound) {
                supportsAllRequiredExtensions = false;
                break;
            }
        }

        auto features = device.getFeatures2<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceVulkan11Features,
            vk::PhysicalDeviceVulkan13Features,
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

        bool supportsRequiredFeatures =
            features.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
            features.get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
            features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
            features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

        if (supportsVulkan1_3 && supportsGraphics &&
            supportsAllRequiredExtensions && supportsRequiredFeatures) {
            std::println("Device: {}", device.getProperties().deviceName.data());
            return device;
        }
    }

    throw std::runtime_error("failed to find a suitable GPU!");
}

}  // namespace

VulkanContext::VulkanContext(WindowApp& windowApp) {
    instance = createInstance(context);
    debugMessenger = setupDebugMessenger(instance);
    surface = windowApp.createSurface(instance);
    physicalDevice = pickPhysicalDevice(instance);
    initLogicalDevice();
    instanceDispatcher = const_cast<decltype(instanceDispatcher)>(instance.getDispatcher());
    deviceDispatcher = const_cast<decltype(deviceDispatcher)>(device.getDispatcher());
    initVmaAllocator();
    // Create command pool
    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = queueFamilyIndex
    };
    commandPool = vk::raii::CommandPool(device, poolInfo);
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };
    this->loadingCmdBuffer =
        std::move(device.allocateCommandBuffers(allocInfo)[0]);
}

vk::raii::detail::InstanceDispatcher* VulkanContext::getStaticInstanceDispatcher() {
    return instanceDispatcher;
}

void VulkanContext::initLogicalDevice() {
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
        physicalDevice.getQueueFamilyProperties();

    // get the first index into queueFamilyProperties which supports both
    // graphics and present
    uint32_t queueIndex = ~0;
    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++) {
        if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
            physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface)) {
            // found a queue family that supports both graphics and present
            queueIndex = qfpIndex;
            break;
        }
    }
    if (queueIndex == ~0) {
        throw std::runtime_error(
            "Could not find a queue for graphics and present -> "
            "terminating"
        );
    }

    // query for Vulkan 1.3 features
    vk::StructureChain featureChain{
        vk::PhysicalDeviceFeatures2{},
        vk::PhysicalDeviceVulkan11Features{.shaderDrawParameters = true},
        vk::PhysicalDeviceVulkan13Features{.synchronization2 = true, .dynamicRendering = true},
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT{.extendedDynamicState = true}
    };

    // create a Device
    float queuePriority = 0.5f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
        .queueFamilyIndex = queueIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };
    vk::DeviceCreateInfo deviceCreateInfo{
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount =
            static_cast<uint32_t>(requiredDeviceExtension.size()),
        .ppEnabledExtensionNames = requiredDeviceExtension.data()
    };

    this->device = vk::raii::Device(physicalDevice, deviceCreateInfo);
    this->queueFamilyIndex = queueIndex;
    this->queue = vk::raii::Queue(device, queueFamilyIndex, 0);
}

void VulkanContext::initVmaAllocator() {
    VmaVulkanFunctions functions = {
        .vkGetInstanceProcAddr =
            [](VkInstance instance, const char* pName) {
                return instanceDispatcher
                    ->vkGetInstanceProcAddr(instance, pName);
            },
        .vkGetDeviceProcAddr =
            [](VkDevice device, const char* pName) {
                return deviceDispatcher
                    ->vkGetDeviceProcAddr(device, pName);
            }
    };
    VmaAllocatorCreateFlags flags =
        VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    VmaAllocatorCreateInfo info{
        .flags = flags,
        .physicalDevice = *physicalDevice,
        .device = *device,
        .pVulkanFunctions = &functions,
        .instance = *instance,
        .vulkanApiVersion = VK_API_VERSION_1_3,
    };
    this->allocator = VmaAllocatorWrapper(new VmaAllocator());
    if (vmaCreateAllocator(&info, allocator.get()) != VK_SUCCESS) {
        throw std::runtime_error("Error creating VMA allocator");
    }
}
