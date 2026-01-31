#include "SimpleModel.hpp"

#include <vector>
#include <vulkan/vulkan.hpp>

#include "ImageLoader.hpp"
#include "core/Model.hpp"
#include "core/RenderPipeline.hpp"
#include "core/Texture.hpp"
#include "core/vertex.hpp"

namespace {

// AI-Generagted Mesh Data
// 24 Vertices (4 per face * 6 faces)
// Positions assume a unit cube from -1.0 to 1.0
// UVs map (0,0) Top-Left to (1,1) Bottom-Right
const std::vector<SimpleVertex> CUBE_VERTICES = {
    // Front Face (+Z)
    // TL, BL, BR, TR
    {{-1.0f, 1.0f, 1.0f}, glm::fvec4{1.0f}, {0.0f, 0.0f}},
    {{-1.0f, -1.0f, 1.0f}, glm::fvec4{1.0f}, {0.0f, 1.0f}},
    {{1.0f, -1.0f, 1.0f}, glm::fvec4{1.0f}, {1.0f, 1.0f}},
    {{1.0f, 1.0f, 1.0f}, glm::fvec4{1.0f}, {1.0f, 0.0f}},

    // Back Face (-Z)
    // TL, BL, BR, TR (Relative to looking AT the face)
    {{1.0f, 1.0f, -1.0f}, glm::fvec4{1.0f}, {0.0f, 0.0f}},
    {{1.0f, -1.0f, -1.0f}, glm::fvec4{1.0f}, {0.0f, 1.0f}},
    {{-1.0f, -1.0f, -1.0f}, glm::fvec4{1.0f}, {1.0f, 1.0f}},
    {{-1.0f, 1.0f, -1.0f}, glm::fvec4{1.0f}, {1.0f, 0.0f}},

    // Left Face (-X)
    {{-1.0f, 1.0f, -1.0f}, glm::fvec4{1.0f}, {0.0f, 0.0f}},
    {{-1.0f, -1.0f, -1.0f}, glm::fvec4{1.0f}, {0.0f, 1.0f}},
    {{-1.0f, -1.0f, 1.0f}, glm::fvec4{1.0f}, {1.0f, 1.0f}},
    {{-1.0f, 1.0f, 1.0f}, glm::fvec4{1.0f}, {1.0f, 0.0f}},

    // Right Face (+X)
    {{1.0f, 1.0f, 1.0f}, glm::fvec4{1.0f}, {0.0f, 0.0f}},
    {{1.0f, -1.0f, 1.0f}, glm::fvec4{1.0f}, {0.0f, 1.0f}},
    {{1.0f, -1.0f, -1.0f}, glm::fvec4{1.0f}, {1.0f, 1.0f}},
    {{1.0f, 1.0f, -1.0f}, glm::fvec4{1.0f}, {1.0f, 0.0f}},

    // Top Face (+Y)
    {{-1.0f, 1.0f, -1.0f}, glm::fvec4{1.0f}, {0.0f, 0.0f}},
    {{-1.0f, 1.0f, 1.0f}, glm::fvec4{1.0f}, {0.0f, 1.0f}},
    {{1.0f, 1.0f, 1.0f}, glm::fvec4{1.0f}, {1.0f, 1.0f}},
    {{1.0f, 1.0f, -1.0f}, glm::fvec4{1.0f}, {1.0f, 0.0f}},

    // Bottom Face (-Y)
    {{-1.0f, -1.0f, 1.0f}, glm::fvec4{1.0f}, {0.0f, 0.0f}},
    {{-1.0f, -1.0f, -1.0f}, glm::fvec4{1.0f}, {0.0f, 1.0f}},
    {{1.0f, -1.0f, -1.0f}, glm::fvec4{1.0f}, {1.0f, 1.0f}},
    {{1.0f, -1.0f, 1.0f}, glm::fvec4{1.0f}, {1.0f, 0.0f}}
};

// 12 Triangles (2 per face)
// Winding order: CCW
struct index {
    uint32_t x, y, z;
};
const std::vector<index> CUBE_INDICES = {
    // Front
    {0, 1, 2},
    {0, 2, 3},
    // Back
    {4, 5, 6},
    {4, 6, 7},
    // Left
    {8, 9, 10},
    {8, 10, 11},
    // Right
    {12, 13, 14},
    {12, 14, 15},
    // Top
    {16, 17, 18},
    {16, 18, 19},
    // Bottom
    {20, 21, 22},
    {20, 22, 23}
};

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
        mesh.vertexBuffer->deleteStagging();
        mesh.indexBuffer->deleteStagging();
        material.texture->deleteStagging();
    }
    return Model{
        std::move(mesh),
        std::move(material)
    };
}
