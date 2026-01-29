#include "SimpleModel.hpp"

#include <vector>
#include <vulkan/vulkan.hpp>

#include "ImageLoader.hpp"
#include "core/Model.hpp"
#include "core/RenderPipeline.hpp"
#include "core/Texture.hpp"
#include "core/vertex.hpp"

namespace {
const std::vector<SimpleVertex> SQUARE_VERTICES = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},  // bottom-left
    {{0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},   // bottom-right
    {{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},    // top-right
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},   // top-left
};

const std::vector<uint32_t> SQUARE_INCICES = {
    0,
    1,
    2,
    2,
    3,
    0,
};

Mesh createMesh(VulkanContext& context) {
    // create buffers
    auto vertBuf = BufferFactory::createStaticBuffer(
        BufferFactory::Type::Vertex,
        *context.allocator,
        getVectorSize(SQUARE_VERTICES)
    );
    auto indexBuf = BufferFactory::createStaticBuffer(
        BufferFactory::Type::Index,
        *context.allocator,
        getVectorSize(SQUARE_INCICES)
    );

    return {
        std::move(vertBuf),
        std::move(indexBuf),
        static_cast<uint32_t>(SQUARE_INCICES.size())
    };
}

Material createMaterial(VulkanContext& context, Image& img) {
    // create render pipeline
    auto shader = readFile("shaders/shader.spv");
    vk::VertexInputBindingDescription bindingDescs[] = {
        SimpleVertex::bindingDescription()
    };
    auto attributeDescs = SimpleVertex::attributeDescriptions();
    auto pipeline = createDefaultGraphicsPipeline(
        context,
        asRawBytes(shader),
        vk::PipelineVertexInputStateCreateInfo{
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = bindingDescs,
            .vertexAttributeDescriptionCount = attributeDescs.size(),
            .pVertexAttributeDescriptions = attributeDescs.data()
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
        .pSetLayouts = &*context.set1Layout
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

Model loadSimpleTraingleModel(VulkanContext& context) {
    auto imgFile = readFile("assets/textures/dog.png");
    auto loadedImg = Image::readImage(imgFile);

    Mesh mesh = createMesh(context);
    Material material = createMaterial(context, loadedImg);

    // Write buffer
    auto& cmd = context.loadingCmdBuffer;
    cmd.begin(
        vk::CommandBufferBeginInfo{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        }
    );
    {
        mesh.vertexBuffer->load(asRawBytes(SQUARE_VERTICES), cmd);
        mesh.indexBuffer->load(asRawBytes(SQUARE_INCICES), cmd);
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
        mesh.vertexBuffer->deleteStagging();
        mesh.indexBuffer->deleteStagging();
        material.texture->deleteStagging();
    }
    return Model{
        std::move(mesh),
        std::move(material)
    };
}
