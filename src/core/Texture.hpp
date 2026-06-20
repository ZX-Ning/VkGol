#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <vk_mem_alloc.h>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "Buffer.hpp"

struct VulkanContext;
struct Image;

struct Texture {
public:
    vk::Image image;
    vk::raii::ImageView view{nullptr};
    vk::raii::Sampler sampler{nullptr};

private:
    VmaAllocation allocation;
    const VmaAllocator& allocator;
    std::unique_ptr<DynamicBuffer> staging;
    vk::Format format;
    vk::Extent3D extend;
    VmaAllocationInfo resultInfo;

public:
    Texture(
        const VulkanContext& context,
        const vk::ImageCreateInfo& info,
        vk::ImageViewCreateInfo viewInfo,
        const vk::SamplerCreateInfo* samplerInfo
    );
    void load(std::span<const uint8_t> data, vk::raii::CommandBuffer& cmd);
    void deleteStaging();
    static constexpr vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding() {
        return {
            0,
            vk::DescriptorType::eCombinedImageSampler,
            1,
            vk::ShaderStageFlagBits::eFragment,
            nullptr
        };
    }
    static constexpr std::array<vk::DescriptorSetLayoutBinding, 1> descriptorSetLayoutBindings() {
        return {
            descriptorSetLayoutBinding()
        };
    }
    // rule of 5:

    ~Texture();
    DISABLE_COPY(Texture)
    Texture(Texture&&) = delete;
    Texture& operator=(Texture&&) = delete;
};

std::shared_ptr<Texture> createDefaultTexture(const VulkanContext& context, const vk::Extent3D& extent);
std::unique_ptr<Texture> createTexture(
    const VulkanContext& context,
    const vk::Extent3D& extent,
    vk::Format format,
    vk::ImageUsageFlags usage,
    vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor,
    bool createSampler = true,
    vk::Filter filter = vk::Filter::eLinear
);
std::unique_ptr<Texture> createNearestTexture(
    const VulkanContext& context,
    const vk::Extent3D& extent,
    vk::Format format,
    vk::ImageUsageFlags usage,
    vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor
);
std::unique_ptr<Texture> createDepthTexture(const VulkanContext& context, const vk::Extent3D& extent);
std::unique_ptr<Texture> createPingPongTexture(const VulkanContext& context, const vk::Extent3D& extent);

#endif  // TEXTURE_HPP
