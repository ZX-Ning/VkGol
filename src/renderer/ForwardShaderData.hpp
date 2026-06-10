#ifndef FORWARD_SHADER_DATA_HPP
#define FORWARD_SHADER_DATA_HPP

#include <array>

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

struct DefaultPushConstant {
    glm::mat4x4 model;
};

struct DefaultSceneUBO {
    glm::mat4x4 view;
    glm::mat4x4 projection;
    static constexpr vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding() {
        return {
            0,
            vk::DescriptorType::eUniformBuffer,
            1,
            vk::ShaderStageFlagBits::eAllGraphics,
            nullptr
        };
    }
    static constexpr std::array<vk::DescriptorSetLayoutBinding, 1> descriptorSetLayoutBindings() {
        return {
            descriptorSetLayoutBinding()
        };
    }
};

#endif  // FORWARD_SHADER_DATA_HPP
