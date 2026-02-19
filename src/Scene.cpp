#include "Scene.hpp"

#include "core/Buffer.hpp"
#include "core/Model.hpp"
#include "core/UniformData.hpp"
#include "core/VulkanContext.hpp"
#include "AppState.hpp"

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

Scene::Scene(std::vector<Ref<RenderObject>>& objs, AppState& state) : state(state) {
    this->objects = objs;
    resetCamera();
    state.currentScene = *this;
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

void Scene::resetCamera() {
    view.eye = {0.f, 4.5f, 10.f};
    view.front = {0.f, -0.5f, -1.f};
    view.up = {0.f, 1.f, 0.f};

    camera.fovy = degreeToRadian(37.8);
    camera.clipNear = 0.1f;
    camera.clipFar = 2000.f;
}
