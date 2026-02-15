#include "Model.hpp"

#include "RenderPipeline.hpp"

void Model::bind(const vk::raii::PipelineLayout& layout,vk::raii::CommandBuffer& cmd) {
    Pipeline& pipeline = *material.pipeline;
    cmd.bindPipeline(
        vk::PipelineBindPoint::eGraphics,
        pipeline.pipeline
    );
    if (material.descriptorSet != nullptr) {
        cmd.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            layout,
            1,
            **material.descriptorSet,
            nullptr
        );
    }
    cmd.bindVertexBuffers(
        0,
        mesh.vertexBuffer->getVkBuffer(),
        {0}
    );
    cmd.bindIndexBuffer(
        mesh.indexBuffer->getVkBuffer(),
        0,
        vk::IndexType::eUint32
    );
}

void Model::render(vk::raii::CommandBuffer& cmd) {
    cmd.drawIndexed(mesh.indexCount, 1, 0, 0, 0);
}
