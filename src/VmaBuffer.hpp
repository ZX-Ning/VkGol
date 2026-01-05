#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <cstring>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "utils.hpp"

class VmaBuffer {
    vk::Buffer buffer;
    VmaAllocation allocation;
    VmaAllocator allocator;
    VmaAllocationInfo resultInfo;

public:
    VmaBuffer() = delete;
    VmaBuffer(
        const vk::BufferCreateInfo& info,
        const VmaAllocationCreateInfo& allocInfo,
        const VmaAllocator& allocator
    ) : allocator(allocator) {
        VkBuffer buffer;
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

    void write(std::span<const uint8_t> data) {
        memcpy(resultInfo.pMappedData, data.data(), data.size_bytes());
    }

    vk::Buffer getVkBuffer() const {
        return buffer;
    }

    // Follow rule of 5
    ~VmaBuffer() {
        if (buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(allocator, buffer, allocation);
        }
    }
    DISABLE_COPY(VmaBuffer)
    VmaBuffer(VmaBuffer&& rhs) = delete;
    VmaBuffer& operator=(VmaBuffer&& rhs) = delete;
};

#endif  // BUFFER_HPP
