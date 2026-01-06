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
    );

public:
    vk::Buffer getVkBuffer() const;
    AbstractVmaBuffer() = delete;
    virtual ~AbstractVmaBuffer();
    DISABLE_COPY(AbstractVmaBuffer)
    AbstractVmaBuffer(AbstractVmaBuffer&&) = delete;
    AbstractVmaBuffer& operator=(AbstractVmaBuffer&&) = delete;
};

class DynamicBuffer : public AbstractVmaBuffer {
public:
    DynamicBuffer(
        const vk::BufferCreateInfo& info,
        const VmaAllocationCreateInfo& allocInfo,
        const VmaAllocator& allocator
    );

    virtual void update(std::span<const uint8_t> data);
};

class StaticBuffer : public AbstractVmaBuffer {
private:
    std::unique_ptr<DynamicBuffer> stagging;
    static void copyBuffer(vk::Buffer src, vk::Buffer dst, vk::raii::CommandBuffer& cmd, vk::DeviceSize size);

public:
    StaticBuffer(
        const vk::BufferCreateInfo& info,
        const VmaAllocationCreateInfo& allocInfo,
        const VmaAllocator& allocator
    );
    virtual void load(std::span<const uint8_t> data, vk::raii::CommandBuffer& cmd);
    void deleteStagging();
};

struct BufferFactory {
    static std::shared_ptr<StaticBuffer> createVertexBuffer(const VmaAllocator allocator, size_t size);
    static std::unique_ptr<DynamicBuffer> createStaggingBuffer(size_t size, VmaAllocator allocator);
};

#endif  // BUFFER_HPP
