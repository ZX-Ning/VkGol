#include "ForwardRenderLayout.hpp"

#include <array>

#include "../core/Texture.hpp"
#include "ForwardShaderData.hpp"
#include "../core/VulkanContext.hpp"

ForwardRenderLayout::ForwardRenderLayout(const VulkanContext& context) {
    sceneSetLayout = vk::raii::DescriptorSetLayout(
        context.device,
        vk::DescriptorSetLayoutCreateInfo{
            .bindingCount = 1,
            .pBindings = DefaultSceneUBO::descriptorSetLayoutBindings().data()
        }
    );
    materialSetLayout = vk::raii::DescriptorSetLayout(
        context.device,
        vk::DescriptorSetLayoutCreateInfo{
            .bindingCount = 1,
            .pBindings = Texture::descriptorSetLayoutBindings().data()
        }
    );

    std::array setLayouts{
        *sceneSetLayout,
        *materialSetLayout,
    };
    vk::PushConstantRange range{
        .stageFlags = vk::ShaderStageFlagBits::eAllGraphics,
        .offset = 0,
        .size = sizeof(DefaultPushConstant),
    };
    pipelineLayout = vk::raii::PipelineLayout(
        context.device,
        vk::PipelineLayoutCreateInfo{
            .setLayoutCount = static_cast<uint32_t>(setLayouts.size()),
            .pSetLayouts = setLayouts.data(),
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &range,
        }
    );
}
