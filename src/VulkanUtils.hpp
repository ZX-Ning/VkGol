#ifndef VULKANUTILS_HPP
#define VULKANUTILS_HPP

#include <vk_mem_alloc.h>

#include <memory>

typedef std::unique_ptr<
    VmaAllocator,
    decltype([](VmaAllocator* ptr) {
        if (ptr) {
            vmaDestroyAllocator(*ptr);
        }
    })>
    VmaAllocatorWrapper;

#endif  // VULKANUTILS_HPP
