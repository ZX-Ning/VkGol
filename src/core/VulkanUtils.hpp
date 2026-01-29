#ifndef VULKANUTILS_HPP
#define VULKANUTILS_HPP

#include <vk_mem_alloc.h>

#include <memory>
#include <vulkan/vulkan.hpp>

struct AllocatorDeleter {
    void operator()(VmaAllocator* ptr) const noexcept {
        if (ptr) {
            vmaDestroyAllocator(*ptr);
        }
    }
};
using VmaAllocatorWrapper = std::unique_ptr<VmaAllocator, AllocatorDeleter>;

inline vk::Format formatToSrgb(vk::Format format) {
    switch (format) {
        case vk::Format::eB8G8R8A8Srgb:
        case vk::Format::eB8G8R8A8Unorm: {
            return vk::Format::eB8G8R8A8Srgb;
        }
        case vk::Format::eR8G8B8A8Srgb:
        case vk::Format::eR8G8B8A8Unorm: {
            return vk::Format::eR8G8B8A8Srgb;
        }
        default: {
            throw std::runtime_error("Format not supported yet.");
        }
    }
}

inline vk::Format formatToUnorm(vk::Format format) {
    switch (format) {
        case vk::Format::eB8G8R8A8Srgb:
        case vk::Format::eB8G8R8A8Unorm: {
            return vk::Format::eB8G8R8A8Unorm;
        }
        case vk::Format::eR8G8B8A8Srgb:
        case vk::Format::eR8G8B8A8Unorm: {
            return vk::Format::eR8G8B8A8Unorm;
        }
        default: {
            throw std::runtime_error("Format not supported yet.");
        }
    }
}

#endif  // VULKANUTILS_HPP
