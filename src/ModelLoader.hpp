#ifndef MODELLOADER_HPP
#define MODELLOADER_HPP

#include "core/Model.hpp"
#include "core/VulkanContext.hpp"

struct ForwardRenderLayout;

struct ModelLoader {
    static Model loadSimpleCubeModel(
        VulkanContext& context,
        const ForwardRenderLayout& layout
    );
};

#endif // MODELLOADER_HPP
