#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "utils.hpp"

class AbstractVmaBuffer {
protected:
    vk::Buffer buffer;
    VmaAllocation allocation;
    VmaAllocator allocator;
    VmaAllocationInfo resultInfo;
    vk::DeviceSize size;
    AbstractVmaBuffer(
        const vk::BufferCreateInfo& info,
        const VmaAllocationCreateInfo& allocInfo,
        const VmaAllocator& allocator
    ) : allocator(allocator) {
        VkBuffer buffer;
        this->size = info.size;
        vmaCreateBuffer(
            allocator,
            &*info,
            &allocInfo,
            &buffer,
            &allocation,
            &resultInfo
        );
        this->buffer = vk::Buffer(buffer);
    }

public:
    vk::Buffer getVkBuffer() const {
        return buffer;
    }

    AbstractVmaBuffer() = delete;
    // Follow rule of 5
    virtual ~AbstractVmaBuffer() {
        if (buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(allocator, buffer, allocation);
        }
    }
    DISABLE_COPY(AbstractVmaBuffer)
    AbstractVmaBuffer(AbstractVmaBuffer&&) = delete;
    AbstractVmaBuffer& operator=(AbstractVmaBuffer&&) = delete;
};

struct DynamicBuffer : public AbstractVmaBuffer {
    DynamicBuffer(
        const vk::BufferCreateInfo& info,
        const VmaAllocationCreateInfo& allocInfo,
        const VmaAllocator& allocator
    ) : AbstractVmaBuffer(info, allocInfo, allocator) {}

    virtual void update(std::span<const uint8_t> data) {
        memcpy(resultInfo.pMappedData, data.data(), data.size_bytes());
    }
};

struct StaticBuffer : public AbstractVmaBuffer {
    std::unique_ptr<DynamicBuffer> stagging;

    StaticBuffer(
        const vk::BufferCreateInfo& info,
        const VmaAllocationCreateInfo& allocInfo,
        const VmaAllocator& allocator
    ) : AbstractVmaBuffer(info, allocInfo, allocator) {}

    static std::unique_ptr<DynamicBuffer> createStagging(
        size_t size, VmaAllocator allocator
    ) {
        vk::BufferCreateInfo stagingInfo{
            .size = size,
            .usage = vk::BufferUsageFlagBits::eTransferSrc,
            .sharingMode = vk::SharingMode::eExclusive,
        };
        VmaAllocationCreateInfo stagingAllocInfo = {
            .flags =
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
        };
        return std::make_unique<DynamicBuffer>(stagingInfo, stagingAllocInfo, allocator);
    }

    static void copyBuffer(vk::Buffer src, vk::Buffer dst, vk::raii::CommandBuffer& cmd, vk::DeviceSize size) {
        cmd.copyBuffer(
            src,
            dst,
            vk::BufferCopy(0, 0, size)
        );
    }

    virtual void load(
        std::span<const uint8_t> data, vk::raii::CommandBuffer& cmd
    ) {
        size_t size = data.size_bytes();
        stagging = createStagging(size, this->allocator);
        stagging->update(data);
        copyBuffer(stagging->getVkBuffer(), this->buffer, cmd, size);
    }

    void deleteStagging() {
        stagging.reset(nullptr);
    }
};

struct BufferFactory {
    static std::shared_ptr<StaticBuffer> createVertexBuffer(
        const VmaAllocator allocator,
        size_t size
    ) {
        // create vertex buffer
        vk::BufferCreateInfo bufferInfo{
            .size = size,
            .usage =
                vk::BufferUsageFlagBits::eVertexBuffer |
                vk::BufferUsageFlagBits::eTransferDst,
            .sharingMode = vk::SharingMode::eExclusive
        };
        VmaAllocationCreateInfo allocInfo = {
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        };
        return std::make_shared<StaticBuffer>(bufferInfo, allocInfo, allocator);
    }
};

#endif  // BUFFER_HPP
