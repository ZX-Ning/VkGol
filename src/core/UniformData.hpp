#ifndef UNIFORMDATA_HPP
#define UNIFORMDATA_HPP

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

struct DefaultPushConstant {
    glm::mat4x4 model;
};

struct DefaultScenceUBO {
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

#endif  // UNIFORMDATA_HPP
