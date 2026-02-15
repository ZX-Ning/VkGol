#ifndef MODELLOADER_HPP
#define MODELLOADER_HPP

#include "core/Model.hpp"
#include "core/VulkanContext.hpp"

struct ModelLoader {
    static Model loadSimpleTraingleModel(VulkanContext& context);
};

#endif // MODELLOADER_HPP
