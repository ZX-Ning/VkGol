#include "Texture.hpp"

#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>

#include "VulkanContext.hpp"

Texture::Texture(
    const VulkanContext& context,
    const vk::ImageCreateInfo& info,
    vk::ImageViewCreateInfo viewInfo,
    const vk::SamplerCreateInfo* samplerInfo
) : allocator(*context.allocator), format(info.format), extend(info.extent) {
    VmaAllocationCreateInfo allocInfo{
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };
    VkImage image;
    if (vmaCreateImage(
            allocator,
            &*info,
            &allocInfo,
            &image,
            &this->allocation,
            &this->resultInfo
        ) != VK_SUCCESS) {
        throw std::runtime_error("Can not create texture");
    }
    this->image = vk::Image(image);
    viewInfo.image = this->image;
    this->view = vk::raii::ImageView(context.device, viewInfo);

    // create sampler
    if (samplerInfo != nullptr) {
        this->sampler = vk::raii::Sampler(context.device, *samplerInfo);
    }
};

void Texture::load(std::span<const uint8_t> data, vk::raii::CommandBuffer& cmd) {
    size_t size = data.size_bytes();
    this->staging = BufferFactory::createStagingBuffer(this->allocator, size);
    this->staging->update(data);

    // transform layout: Undefined -> TransferDst
    {
        vk::ImageMemoryBarrier2 barrier = {
            .srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe,
            .srcAccessMask = {},
            .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
            .dstAccessMask = vk::AccessFlagBits2::eTransferWrite,
            .oldLayout = vk::ImageLayout::eUndefined,
            .newLayout = vk::ImageLayout::eTransferDstOptimal,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = this->image,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        cmd.pipelineBarrier2(
            vk::DependencyInfo{
                .dependencyFlags = {},
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers = &barrier
            }
        );
    }
    // copy from staging buffer to image
    {
        vk::BufferImageCopy region{
            0,
            0,
            0,
            {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
            {0, 0, 0},
            extend
        };
        cmd.copyBufferToImage(
            staging->getVkBuffer(),
            image,
            vk::ImageLayout::eTransferDstOptimal,
            region
        );
    }
    // transform layout: TransferDst -> ShaderReadOnly
    {
        vk::ImageMemoryBarrier2 barrier = {
            .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
            .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
            .dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
            .dstAccessMask = vk::AccessFlagBits2::eShaderRead,
            .oldLayout = vk::ImageLayout::eTransferDstOptimal,
            .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = this->image,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        cmd.pipelineBarrier2(
            vk::DependencyInfo{
                .dependencyFlags = {},
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers = &barrier
            }
        );
    }
}

void Texture::deleteStaging() {
    staging = nullptr;
}

Texture::~Texture() {
    view.clear();
    if (image != VK_NULL_HANDLE) {
        vmaDestroyImage(allocator, image, allocation);
    }
}

std::shared_ptr<Texture> createDefaultTexture(
    const VulkanContext& context, const vk::Extent3D& extent
) {
    // create image
    auto format = vk::Format::eR8G8B8A8Srgb;
    vk::ImageCreateInfo imageInfo{
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = extent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        .sharingMode = vk::SharingMode::eExclusive
    };

    vk::ImageViewCreateInfo viewInfo{
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange = {
            vk::ImageAspectFlagBits::eColor,
            0,
            1,
            0,
            1
        }
    };

    vk::PhysicalDeviceProperties properties = context.physicalDevice.getProperties();
    vk::SamplerCreateInfo samplerInfo{
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .mipLodBias = 0.0f,
        .anisotropyEnable = vk::False,
        .compareEnable = vk::False,
        .compareOp = vk::CompareOp::eAlways
    };

    return std::make_shared<Texture>(
        context,
        imageInfo,
        viewInfo,
        &samplerInfo
    );
}

std::unique_ptr<Texture> createTexture(
    const VulkanContext& context,
    const vk::Extent3D& extent,
    vk::Format format,
    vk::ImageUsageFlags usage,
    vk::ImageAspectFlags aspectMask,
    bool createSampler
) {
    vk::ImageCreateInfo imageInfo{
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = extent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive
    };

    vk::ImageViewCreateInfo viewInfo{
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange = {
            aspectMask,
            0,
            1,
            0,
            1
        }
    };

    vk::SamplerCreateInfo samplerInfo{
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eClampToEdge,
        .addressModeV = vk::SamplerAddressMode::eClampToEdge,
        .addressModeW = vk::SamplerAddressMode::eClampToEdge,
        .mipLodBias = 0.0f,
        .anisotropyEnable = vk::False,
        .compareEnable = vk::False,
        .compareOp = vk::CompareOp::eAlways
    };

    return std::make_unique<Texture>(
        context,
        imageInfo,
        viewInfo,
        createSampler ? &samplerInfo : nullptr
    );
}

std::unique_ptr<Texture> createDepthTexture(const VulkanContext& context, const vk::Extent3D& extent) {
    return createTexture(
        context,
        extent,
        vk::Format::eD32Sfloat,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::ImageAspectFlagBits::eDepth,
        false
    );
}

std::unique_ptr<Texture> createPingPongTexture(const VulkanContext& context, const vk::Extent3D& extent) {
    return createTexture(
        context,
        extent,
        vk::Format::eR16G16B16A16Sfloat,
        vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eSampled |
            vk::ImageUsageFlagBits::eStorage
    );
}
