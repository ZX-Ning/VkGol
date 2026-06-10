#include "ModelLoader.hpp"

#include <vector>
#include <vulkan/vulkan.hpp>

#include "ImageLoader.hpp"
#include "core/Model.hpp"
#include "core/RenderPipeline.hpp"
#include "core/Texture.hpp"
#include "core/vertex.hpp"
#include "Consts.hpp"
#include "CubeMesh.hpp"
#include "renderer/ForwardRenderLayout.hpp"

namespace {
Mesh createMesh(VulkanContext& context) {
    // create buffers
    auto vertBuf = BufferFactory::createStaticBuffer(
        BufferFactory::Type::Vertex,
        *context.allocator,
        getVectorSize(CUBE_VERTICES)
    );
    auto indexBuf = BufferFactory::createStaticBuffer(
        BufferFactory::Type::Index,
        *context.allocator,
        getVectorSize(CUBE_INDICES)
    );

    return {
        std::move(vertBuf),
        std::move(indexBuf),
        static_cast<uint32_t>(CUBE_INDICES.size() * 3)
    };
}

Material createMaterial(
    VulkanContext& context,
    const ForwardRenderLayout& layout,
    Image& img
) {
    // create render pipeline
    auto shader = readFile(Consts::SHADER_SPV_PATH);
    vk::VertexInputBindingDescription bindingDescs[] = {
        SimpleVertex::bindingDescription()
    };
    auto attributeDescs = SimpleVertex::attributeDescriptions();
    auto pipeline = createGraphicsPipeline(
        context,
        GraphicsPipelineDesc{
            .layout = *layout.pipelineLayout,
            .shaderSpv = asRawBytes(shader),
            .vertexInfo = vk::PipelineVertexInputStateCreateInfo{
                .vertexBindingDescriptionCount = 1,
                .pVertexBindingDescriptions = bindingDescs,
                .vertexAttributeDescriptionCount = attributeDescs.size(),
                .pVertexAttributeDescriptions = attributeDescs.data()
            },
            .colorFormat = context.surfaceFormat.format,
            .depthFormat = vk::Format::eD32Sfloat
        }
    );

    // create texture (with sampler)

    auto texture = createDefaultTexture(
        context,
        vk::Extent3D{
            static_cast<uint32_t>(img.width),
            static_cast<uint32_t>(img.height),
            1
        }
    );

    // create descriptor set
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = context.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &*layout.materialSetLayout
    };
    auto sets =
        context.device.allocateDescriptorSets(allocInfo);
    assert(sets.size() == 1);
    auto set = std::make_shared<vk::raii::DescriptorSet>(std::move(sets[0]));
    sets.clear();

    vk::DescriptorImageInfo imageInfo{
        .sampler = texture->sampler,
        .imageView = texture->view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };
    std::array descriptorWrites{
        vk::WriteDescriptorSet{
            .dstSet = *set,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &imageInfo
        }
    };
    context.device.updateDescriptorSets(descriptorWrites, {});

    return {
        std::move(pipeline),
        std::move(texture),
        std::move(set)
    };
}

}  // namespace

Model ModelLoader::loadSimpleCubeModel(
    VulkanContext& context,
    const ForwardRenderLayout& layout
) {
    auto imgFile = readFile(Consts::DOG_TEXTURE_PATH);
    auto loadedImg = Image::readImage(imgFile);

    Mesh mesh = createMesh(context);
    Material material = createMaterial(context, layout, loadedImg);

    // Write buffer
    auto& cmd = context.loadingCmdBuffer;
    cmd.begin(
        vk::CommandBufferBeginInfo{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        }
    );
    {
        mesh.vertexBuffer->load(asRawBytes(CUBE_VERTICES), cmd);
        mesh.indexBuffer->load(asRawBytes(CUBE_INDICES), cmd);
        material.texture->load(loadedImg.rawData(), cmd);
        cmd.end();
    }

    const vk::SubmitInfo submitInfo{
        .commandBufferCount = 1,
        .pCommandBuffers = &*cmd,
    };
    context.queue.submit(submitInfo);
    context.queue.waitIdle();
    {
        mesh.vertexBuffer->deleteStaging();
        mesh.indexBuffer->deleteStaging();
        material.texture->deleteStaging();
    }
    return Model{
        std::move(mesh),
        std::move(material)
    };
}
