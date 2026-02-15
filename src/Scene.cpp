#include "Scene.hpp"

#include "core/Buffer.hpp"
#include "core/Model.hpp"
#include "core/UniformData.hpp"
#include "core/VulkanContext.hpp"

glm::fmat4x4 RenderObject::calcModelMatrix() const {
    glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 r = glm::mat4_cast(glm::angleAxis(angle, glm::normalize(axis)));
    glm::mat4 s = glm::scale(glm::mat4(1.f), scale);

    return t * r * s;
}

void RenderObject::render(const Layouts& layouts, vk::raii::CommandBuffer& cmd) const {
    model.bind(layouts.pipelineLayout, cmd);
    DefaultPushConstant pc{glm::transpose(calcModelMatrix())};
    cmd.pushConstants(
        layouts.pipelineLayout,
        vk::ShaderStageFlagBits::eAllGraphics,
        0,
        vk::ArrayProxy<const DefaultPushConstant>{1, &pc}
    );
    model.render(cmd);
}

void Scene::render(
    const Layouts& layouts,
    vk::raii::CommandBuffer& cmd,
    DynamicBuffer& ub,
    float ratio
) {
    camera.aspectRatio = ratio;
    DefaultScenceUBO ubo{
        glm::transpose(view.calcMat()),
        glm::transpose(camera.clacMat())
    };
    ub.update(objectAsRawBytes(ubo));
    for (const RenderObject& obj : objects) {
        obj.render(layouts, cmd);
    }
}
