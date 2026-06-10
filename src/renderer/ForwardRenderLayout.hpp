#ifndef FORWARD_RENDER_LAYOUT_HPP
#define FORWARD_RENDER_LAYOUT_HPP

#include <vulkan/vulkan_raii.hpp>

struct VulkanContext;

struct ForwardRenderLayout {
    vk::raii::DescriptorSetLayout sceneSetLayout{nullptr};
    vk::raii::DescriptorSetLayout materialSetLayout{nullptr};
    vk::raii::PipelineLayout pipelineLayout{nullptr};

    explicit ForwardRenderLayout(const VulkanContext& context);
};

#endif  // FORWARD_RENDER_LAYOUT_HPP
