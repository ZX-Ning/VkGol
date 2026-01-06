#include "VulkanApp.hpp"

// std c++
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <print>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

// vulkan
#include <vulkan/vulkan_core.h>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_structs.hpp>
#include <vulkan/vulkan_to_string.hpp>

// imgui
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

// project
#include "VmaBuffer.hpp"
#include "WindowApp.hpp"
#include "backends/imgui_impl_glfw.h"
#include "utils.hpp"
#include "vertex.hpp"

namespace {
// Static pointer to dispatchers to make Imgui and VMA work,
// Ugly but can not found other solutions.
// Require only one VulkanApp at a time.
static vk::raii::detail::InstanceDispatcher* instanceDispatcher;
static vk::raii::detail::DeviceDispatcher* deviceDispatcher;

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
        requiredLayers.assign(validationLayers.begin(), validationLayers.end());
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

std::tuple<vk::raii::Device, uint32_t> createLogicalDeviceAndQueueIndex(
    const vk::raii::PhysicalDevice& physicalDevice, const vk::raii::SurfaceKHR& surface
) {
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

    vk::raii::Device device(physicalDevice, deviceCreateInfo);
    // globalDeviceForImgui = &device;
    return {std::move(device), queueIndex};
}

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

VulkanApp::SwapChain createSwapChain(
    const vk::raii::PhysicalDevice& physicalDevice,
    const vk::raii::Device& device,
    const vk::raii::SurfaceKHR& surface,
    uint32_t minImageCount,
    vk::Extent2D fbSize
) {
    auto surfaceCapabilities =
        physicalDevice.getSurfaceCapabilitiesKHR(surface);
    vk::Extent2D swapChainExtent = chooseSwapExtent(surfaceCapabilities, fbSize);
    vk::SurfaceFormatKHR swapChainSurfaceFormat = chooseSwapSurfaceFormat(
        physicalDevice.getSurfaceFormatsKHR(surface)
    );
    vk::SwapchainCreateInfoKHR swapChainCreateInfo{
        .surface = surface,
        .minImageCount = minImageCount,
        .imageFormat = swapChainSurfaceFormat.format,
        .imageColorSpace = swapChainSurfaceFormat.colorSpace,
        .imageExtent = swapChainExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = chooseSwapPresentMode(
            physicalDevice.getSurfacePresentModesKHR(surface)
        ),
        .clipped = true
    };

    vk::raii::SwapchainKHR swapChain{device, swapChainCreateInfo};
    auto swapChainImages = swapChain.getImages();

    vk::ImageViewCreateInfo imageViewCreateInfo{
        .viewType = vk::ImageViewType::e2D,
        .format = swapChainSurfaceFormat.format,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    std::vector<VulkanApp::SurfaceImages> views;
    for (auto& image : swapChainImages) {
        imageViewCreateInfo.image = image;
        views.emplace_back(
            std::move(image),
            vk::raii::ImageView{device, imageViewCreateInfo},
            vk::raii::Semaphore{device, vk::SemaphoreCreateInfo{}}
        );
    }

    return {
        .swapChain = std::move(swapChain),
        .surfaceFormat = swapChainSurfaceFormat,
        .extent = swapChainExtent,
        .images = std::move(views),
    };
}

void transitionImageLayout(
    const vk::Image& image,
    const vk::raii::CommandBuffer& commandBuffer,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    vk::AccessFlags2 src_access_mask,
    vk::AccessFlags2 dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask
) {
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = src_stage_mask,
        .srcAccessMask = src_access_mask,
        .dstStageMask = dst_stage_mask,
        .dstAccessMask = dst_access_mask,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    vk::DependencyInfo dependency_info = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };
    commandBuffer.pipelineBarrier2(dependency_info);
}

vk::raii::ShaderModule createShaderModule(const vk::raii::Device& device, const std::span<char> spv) {
    vk::ShaderModuleCreateInfo createInfo{
        .codeSize = spv.size() * sizeof(char),
        .pCode = reinterpret_cast<const uint32_t*>(spv.data())
    };
    vk::raii::ShaderModule shaderModule{device, createInfo};

    return shaderModule;
}

vk::raii::Pipeline createGraphicsPipeline(
    const vk::raii::Device& device, vk::SurfaceFormatKHR surfaceFormat
) {
    std::vector<char> spv = readFile("shaders/shader.spv");
    vk::raii::ShaderModule shaderModule =
        createShaderModule(device, spv);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = shaderModule,
        .pName = "vertMain"
    };
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = shaderModule,
        .pName = "fragMain"
    };
    vk::PipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageInfo, fragShaderStageInfo
    };

    auto bindingDescription = SimpleVertex::bindingDescription();
    auto attributeDescriptions = SimpleVertex::attributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = attributeDescriptions.size(),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
    };
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        .topology = vk::PrimitiveTopology::eTriangleList
    };
    vk::PipelineViewportStateCreateInfo viewportState{
        .viewportCount = 1, .scissorCount = 1
    };

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eClockwise,
        .depthBiasEnable = vk::False,
        .depthBiasSlopeFactor = 1.0f,
        .lineWidth = 1.0f
    };

    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR |
                          vk::ColorComponentFlagBits::eG |
                          vk::ColorComponentFlagBits::eB |
                          vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = vk::False,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };

    std::vector dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicState{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount = 0, .pushConstantRangeCount = 0
    };

    vk::raii::PipelineLayout pipelineLayout(device, pipelineLayoutInfo);

    vk::StructureChain pipelineCreateInfoChain{
        vk::GraphicsPipelineCreateInfo{
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamicState,
            .layout = pipelineLayout,
            .renderPass = nullptr
        },
        vk::PipelineRenderingCreateInfo{
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &surfaceFormat.format
        }
    };

    return {
        device,
        nullptr,
        pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>()
    };
}

/*
 * updates: pool, frames, loadingBuf
 */
void createCmdBuffers(
    vk::raii::CommandPool& pool,
    std::vector<VulkanApp::Frame>& frames,
    vk::raii::CommandBuffer& loadingBuf,
    const vk::raii::Device& device,
    uint32_t queueFamilyIndex
) {
    assert(frames.empty());
    if (pool == nullptr) {
        vk::CommandPoolCreateInfo poolInfo{
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = queueFamilyIndex
        };
        pool = vk::raii::CommandPool(device, poolInfo);
    }
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };
    vk::raii::CommandBuffers commandbufs{device, allocInfo};

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        frames.emplace_back(
            std::move(commandbufs[i]),
            vk::raii::Semaphore{device, vk::SemaphoreCreateInfo{}},
            vk::raii::Fence{
                device,
                vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled}
            }
        );
    }
    // Create loading buffer
    vk::CommandBufferAllocateInfo allocInfo2{
        .commandPool = pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };
    loadingBuf = std::move(device.allocateCommandBuffers(allocInfo)[0]);
}

uint32_t findMemoryType(
    const vk::raii::PhysicalDevice& physicalDevice,
    uint32_t typeFilter,
    vk::MemoryPropertyFlags properties
) {
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

std::shared_ptr<StaticBuffer> createVertexBuffer(
    const VmaAllocator& allocator
) {
    const auto& vertices = TRAINGLE;
    // create vertex buffer
    return BufferFactory::createVertexBuffer(allocator, getVectorSize(TRAINGLE));
}

void writeVertexBuffer(
    StaticBuffer& vertBuffer,
    vk::raii::Queue& queue,
    vk::raii::CommandBuffer& cmd
) {
    const auto& vertices = TRAINGLE;
    cmd.begin(
        vk::CommandBufferBeginInfo{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        }
    );
    vertBuffer.load(asRawBytes(TRAINGLE), cmd);
    cmd.end();

    const vk::SubmitInfo submitInfo{
        .commandBufferCount = 1,
        .pCommandBuffers = &*cmd,
    };
    queue.submit(submitInfo);
    queue.waitIdle();
}

void drawImgui(vk::raii::CommandBuffer& buffer, VulkanApp::AppState& state) {
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
    {
        auto imguiScale = ImGui::GetIO().DisplayFramebufferScale;
        auto size = ImGui::GetMainViewport()->Size;
        // std::println("Imgui scale: {}x{}", imguiScale.x, imguiScale.y);
        ImGui::SetNextWindowSize(ImVec2(size.x * 0.75, size.y * 0.15), ImGuiCond_Appearing);
        ImGui::Begin("Hello, world!");
        ImGui::ColorEdit3(
            "clear color",
            state.clearColor.getRaw()
        );
        ImGui::SameLine();
        ImGui::Checkbox("Demo Window", &state.showDemoWindow);
        ImGui::Text("Frame time: %.2fms", state.frameTime);
        ImGui::End();
    }
    if (state.showDemoWindow) {
        ImGui::ShowDemoWindow(&(state.showDemoWindow));
    }
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(draw_data, *buffer);
}

VmaAllocatorWrapper createVmaAllocator(
    const vk::raii::Instance& instance,
    const vk::raii::PhysicalDevice& physicalDevice,
    const vk::raii::Device& device
) {
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
    VmaAllocator allocator;
    if (vmaCreateAllocator(&info, &allocator) != VK_SUCCESS) {
        throw std::runtime_error("Error creating VMA allocator");
    }
    VmaAllocatorWrapper wrapper(new VmaAllocator);
    *wrapper = allocator;
    return wrapper;
}

// TODO: ADD MORE FUNCTION HERE
}  // namespace

void VulkanApp::init() {
    instance = createInstance(context);
    debugMessenger = setupDebugMessenger(instance);
    surface = windowApp->createSurface(instance);
    physicalDevice = pickPhysicalDevice(instance);
    {  // creat logic device and queue
        auto result = createLogicalDeviceAndQueueIndex(physicalDevice, surface);
        device = std::move(std::get<0>(result));
        queueFamilyIndex = std::get<1>(result);
        queue = vk::raii::Queue(device, queueFamilyIndex, 0);
    }
    instanceDispatcher = const_cast<decltype(instanceDispatcher)>(instance.getDispatcher());
    deviceDispatcher = const_cast<decltype(deviceDispatcher)>(device.getDispatcher());

    allocator = createVmaAllocator(instance, physicalDevice, device);

    Size2D<uint32_t> size = windowApp->getFrameSize();
    minImageCount = chooseSwapMinImageCount(
        physicalDevice.getSurfaceCapabilitiesKHR(surface)
    );

    swapChain = createSwapChain(
        physicalDevice, device, surface, minImageCount, {size.width, size.height}
    );

    graphicsPipeline = createGraphicsPipeline(device, swapChain.surfaceFormat);
    createCmdBuffers(
        commandPool,
        frames,
        loadingCmdBuffer,
        device,
        queueFamilyIndex
    );

    initImgui();

    vertexBuffer = createVertexBuffer(*allocator);
    writeVertexBuffer(*vertexBuffer, queue, loadingCmdBuffer);

    state.lastRenderTimestamp = getTimestampMs();
}

void VulkanApp::initImgui() {
    ImGuiIO* imguiIo = &ImGui::GetIO();
    imguiIo->ConfigFlags |= ImGuiConfigFlags_IsSRGB;
    imguiIo->IniFilename = NULL;

    // font
    ImFontConfig fontcfg;
    fontcfg.OversampleH = 2;
    fontcfg.OversampleV = 2;
    fontcfg.PixelSnapH = true;
    fontcfg.PixelSnapV = true;
    fontcfg.RasterizerDensity = 0.86f;
    imguiIo->Fonts->AddFontFromFileTTF(
        "assets/fonts/IBMPlex/IBMPlexSans-Regular.ttf",
        17.f,
        &fontcfg
    );
    ImGuiStyle* style = &ImGui::GetStyle();
    auto size = windowApp->getFrameSize();
    // set style
    ImGui::StyleColorsDark();
    style->WindowRounding = 5.f;
    style->FrameRounding = 5.f;
    style->WindowPadding = {8, 5};
    style->FramePadding = {3, 2};

    float scale = windowApp->getScale();
    if (windowApp->scalingType == WindowApp::WINDOWS_OR_X11) {
        style->FontScaleDpi = scale;
        style->ScaleAllSizes(scale);
    }

    vk::PipelineRenderingCreateInfoKHR pipelineInfo{
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapChain.surfaceFormat.format,
    };

    auto vert = readFile("shaders/imgui/vert.spv");
    vk::ShaderModuleCreateInfo vertInfo{
        .codeSize = getVectorSize(vert),
        .pCode = reinterpret_cast<uint32_t*>(vert.data())
    };
    auto frag = readFile("shaders/imgui/frag.spv");
    vk::ShaderModuleCreateInfo fragInfo{
        .codeSize = getVectorSize(frag),
        .pCode = reinterpret_cast<uint32_t*>(frag.data())
    };

    ImGui_ImplVulkan_InitInfo initInfo{
        .ApiVersion = VK_API_VERSION_1_3,
        .Instance = *instance,
        .PhysicalDevice = *physicalDevice,
        .Device = *device,
        .QueueFamily = queueFamilyIndex,
        .Queue = *queue,
        .DescriptorPoolSize = 1 << 4,
        .MinImageCount = minImageCount,
        .ImageCount = static_cast<uint32_t>(swapChain.images.size()),
        .PipelineInfoMain = {
            .PipelineRenderingCreateInfo = *pipelineInfo,
        },
        .UseDynamicRendering = true,
        .CustomShaderVertCreateInfo = vertInfo,
        .CustomShaderFragCreateInfo = fragInfo
    };

    ImGui_ImplVulkan_LoadFunctions(
        VK_API_VERSION_1_3,
        [](const char* name, void* data) {
            const vk::Instance* instance =
                reinterpret_cast<const vk::Instance*>(data);
            auto dispatcher = instanceDispatcher;
            return dispatcher->vkGetInstanceProcAddr(*instance, name);
        },
        const_cast<void*>(reinterpret_cast<const void*>(&*instance))
    );

    ImGui_ImplVulkan_Init(&initInfo);
};

void VulkanApp::recreateSwapChain() {
    Size2D<uint32_t> size = windowApp->getFrameSize();
    if (windowApp->isMinimized() || size.height <= 0 || size.width <= 0) {
        return;
    }
    device.waitIdle();
    swapChain.reset();
    minImageCount = chooseSwapMinImageCount(physicalDevice.getSurfaceCapabilitiesKHR(surface));
    swapChain = createSwapChain(
        physicalDevice, device, surface, minImageCount, {size.width, size.height}
    );
}

void VulkanApp::drawFrame() {
    if (windowApp->isMinimized()) {
        this->framebufferResized = true;
        // std::println("Minimized, skip rendering");
        return;
    }
    auto size = windowApp->getFrameSize();
    ImGui::GetIO().DisplaySize = ImVec2{size.width * 1.f, size.height * 1.f};
    uint64_t timeNow = getTimestampMs();
    state.frameTime = timeNow - state.lastRenderTimestamp;
    state.lastRenderTimestamp = timeNow;
    // Note: inFlightFences, presentCompleteSemaphores, and commandBuffers
    // are indexed by frameIndex, while renderFinishedSemaphores is indexed by imageIndex
    auto& frame = frames[frameIndex];

    vk::Result fenceResult = device.waitForFences(
        *frame.fences, vk::True, UINT64_MAX
    );
    if (fenceResult != vk::Result::eSuccess) {
        throw std::runtime_error("failed to wait for fence!");
    }
    device.resetFences(*frame.fences);

    auto [result, imageIndex] = swapChain.swapChain.acquireNextImage(
        UINT64_MAX,
        *frame.presentComplete,
        nullptr
    );

    if (result == vk::Result::eErrorOutOfDateKHR) {
        recreateSwapChain();
        return;
    }
    if (result != vk::Result::eSuccess &&
        result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    auto& image = swapChain.images[imageIndex];
    frame.cmdBuffer.reset();
    auto width = static_cast<float>(swapChain.extent.width);
    auto height = static_cast<float>(swapChain.extent.height);

    // record commandBuffer
    {
        frame.cmdBuffer.begin({});
        // Before starting rendering, transition the swapchain image to
        // COLOR_ATTACHMENT_OPTIMAL
        transitionImageLayout(
            image.image,
            frame.cmdBuffer,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            {},  // srcAccessMask (no need to wait for previous operations)
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput
        );
        vk::ClearValue clearColor = {state.clearColor.srgbToLinear()};
        vk::RenderingAttachmentInfo attachmentInfo = vk::RenderingAttachmentInfo{
            .imageView = image.imageView,
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clearColor
        };
        vk::RenderingInfo renderingInfo = {
            .renderArea = {
                .offset = {0, 0},
                .extent = swapChain.extent
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &attachmentInfo
        };
        frame.cmdBuffer.beginRendering(renderingInfo);
        frame.cmdBuffer.setViewport(
            0, vk::Viewport(0.0f, 0.0f, width, height, 0.0f, 1.0f)
        );
        frame.cmdBuffer.setScissor(
            0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.extent)
        );
        frame.cmdBuffer.bindPipeline(
            vk::PipelineBindPoint::eGraphics,
            *graphicsPipeline
        );
        frame.cmdBuffer.bindVertexBuffers(
            0, vertexBuffer->getVkBuffer(), {0}
        );
        frame.cmdBuffer.draw(3, 1, 0, 0);
        drawImgui(frame.cmdBuffer, state);
        frame.cmdBuffer.endRendering();
        // After rendering, transition the swapchain image to PRESENT_SRC
        transitionImageLayout(
            image.image,
            frame.cmdBuffer,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            {},
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eBottomOfPipe
        );
        frame.cmdBuffer.end();
    }

    vk::PipelineStageFlags waitDestinationStageMask(
        vk::PipelineStageFlagBits::eColorAttachmentOutput
    );
    const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*frame.presentComplete,
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*frame.cmdBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*image.renderComplete,
    };
    queue.submit(submitInfo, *frame.fences);

    try {
        const vk::PresentInfoKHR presentInfoKHR{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*image.renderComplete,
            .swapchainCount = 1,
            .pSwapchains = &(*swapChain.swapChain),
            .pImageIndices = &imageIndex
        };
        result = queue.presentKHR(presentInfoKHR);
        if (result == vk::Result::eSuboptimalKHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
        }
        else if (result != vk::Result::eSuccess) {
            throw std::runtime_error("failed to present swap chain image!");
        }
    }
    catch (const vk::SystemError& e) {
        int width, height;
        if (e.code().value() == static_cast<int>(vk::Result::eErrorOutOfDateKHR)) {
            recreateSwapChain();
            return;
        }
        else {
            throw;
        }
    }
    frameIndex++;
    frameIndex %= MAX_FRAMES_IN_FLIGHT;
}

VulkanApp::VulkanApp(std::unique_ptr<WindowApp>&& window)
    : windowApp(std::move(window)) {
    init();
}

void VulkanApp::run() {
    windowApp->cleanupCallBack =
        [this]() {
            this->device.waitIdle();
            ImGui_ImplVulkan_Shutdown();
        };
    windowApp->resizeCallBack =
        [this](int, int) { this->framebufferResized = true; };
    windowApp->drawFrameCallBack =
        [this]() { this->drawFrame(); };
    windowApp->run();
}
