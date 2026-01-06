#include "VmaBuffer.hpp"

AbstractVmaBuffer::AbstractVmaBuffer(
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

vk::Buffer AbstractVmaBuffer::getVkBuffer() const {
    return buffer;
}

// Follow rule of 5
AbstractVmaBuffer::~AbstractVmaBuffer() {
    if (buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(allocator, buffer, allocation);
    }
}

DynamicBuffer::DynamicBuffer(
    const vk::BufferCreateInfo& info,
    const VmaAllocationCreateInfo& allocInfo,
    const VmaAllocator& allocator
) : AbstractVmaBuffer(info, allocInfo, allocator) {}

void DynamicBuffer::update(std::span<const uint8_t> data) {
    memcpy(resultInfo.pMappedData, data.data(), data.size_bytes());
}

StaticBuffer::StaticBuffer(
    const vk::BufferCreateInfo& info,
    const VmaAllocationCreateInfo& allocInfo,
    const VmaAllocator& allocator
) : AbstractVmaBuffer(info, allocInfo, allocator) {}

void StaticBuffer::copyBuffer(vk::Buffer src, vk::Buffer dst, vk::raii::CommandBuffer& cmd, vk::DeviceSize size) {
    cmd.copyBuffer(
        src,
        dst,
        vk::BufferCopy(0, 0, size)
    );
}

void StaticBuffer::load(
    std::span<const uint8_t> data, vk::raii::CommandBuffer& cmd
) {
    size_t size = data.size_bytes();
    stagging = BufferFactory::createStaggingBuffer(size, this->allocator);
    stagging->update(data);
    copyBuffer(stagging->getVkBuffer(), this->buffer, cmd, size);
}

void StaticBuffer::deleteStagging() {
    stagging.reset(nullptr);
}

std::shared_ptr<StaticBuffer> BufferFactory::createVertexBuffer(
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

std::unique_ptr<DynamicBuffer> BufferFactory::createStaggingBuffer(
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
