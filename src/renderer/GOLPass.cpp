#include "GOLPass.hpp"

#include <vulkan/vulkan_raii.hpp>

#include "../core/Buffer.hpp"
#include "../core/RenderPipeline.hpp"
#include "../core/Texture.hpp"
#include "../core/VulkanUtils.hpp"

constexpr int GROUP_SIZE = 16;

static void WriteDescriptorSet(
    std::vector<const LoadedBuffer*> bufs,
    const vk::DescriptorSet& set,
    const vk::raii::Device& device
) {
    std::vector<vk::DescriptorBufferInfo> infos;
    std::vector<vk::WriteDescriptorSet> writes;
    for (size_t i = 0; i < bufs.size(); i++) {
        infos.push_back({
            .buffer = bufs[i]->getVkBuffer(),
            .offset = 0UL,
            .range = bufs[i]->size(),
        });
    }
    for (size_t i = 0; i < bufs.size(); i++) {
        writes.push_back(
            vk::WriteDescriptorSet{
                .dstSet = set,
                .dstBinding = static_cast<uint32_t>(i),
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
            }
                .setBufferInfo({infos[i]})
        );
    }
    device.updateDescriptorSets(writes, {});
}

GOLPass::GOLPass(const VulkanContext& ctx, uint32_t golSize)
    : golSize(golSize) {
    // --------------------------------------------
    // ------ CREATE LAYOUTS-----------------------
    // --------------------------------------------

    std::array layoutBindings = {
        vk::DescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
        vk::DescriptorSetLayoutBinding{
            .binding = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        }
    };
    this->setLayout = ctx.device.createDescriptorSetLayout(
        vk::DescriptorSetLayoutCreateInfo{}.setBindings(layoutBindings)
    );
    vk::PushConstantRange pcRange = {
        vk::ShaderStageFlagBits::eCompute, 0, 2 * sizeof(uint32_t)
    };

    this->pipelineLayout = ctx.device.createPipelineLayout(
        vk::PipelineLayoutCreateInfo{}
            .setSetLayouts({*setLayout})
            .setPushConstantRanges({pcRange})
    );

    // ---------------------------------------------
    // ------ CREATE TEXTURE AND BUFFERS------------
    // ---------------------------------------------

    this->golData[0] = BufferFactory::createStaticBuffer(
        BufferFactory::Type::Storage, *ctx.allocator, static_cast<size_t>(golSize) * golSize
    );
    this->golData[1] = BufferFactory::createStaticBuffer(
        BufferFactory::Type::Storage, *ctx.allocator, static_cast<size_t>(golSize) * golSize
    );
    this->targetTexture = createNearestTexture(
        ctx,
        {golSize, golSize, 1},
        vk::Format::eR8Unorm,
        vk::ImageUsageFlagBits::eTransferDst |
            vk::ImageUsageFlagBits::eSampled
    );

    // ---------------------------------------------
    // ------ CREATE PIPEPINE AND SET --------------
    // ---------------------------------------------

    this->updatePipe = createComputePipeline(
        ctx,
        {.layout = pipelineLayout, .shaderSpv = readFile("shaders/gol.spv")}
    );
    auto setLayouts = std::array{*setLayout, *setLayout};
    auto setTemp = ctx.device.allocateDescriptorSets(
        vk::DescriptorSetAllocateInfo{.descriptorPool = ctx.descriptorPool}
            .setSetLayouts(setLayouts)
    );
    this->bindingSets[0] = std::move(setTemp[0]);
    this->bindingSets[1] = std::move(setTemp[1]);
    WriteDescriptorSet(
        {golData[0].get(), golData[1].get()},
        bindingSets[0],
        ctx.device
    );
    WriteDescriptorSet(
        {golData[1].get(), golData[0].get()},
        bindingSets[1],
        ctx.device
    );
}

void GOLPass::record(vk::raii::CommandBuffer& cmd) {
    auto dstBuf = this->golData[current ^ 1]->getVkBuffer();

    cmd.bindPipeline(
        vk::PipelineBindPoint::eCompute,
        this->updatePipe->pipeline
    );
    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute,
        pipelineLayout,
        0,
        {*bindingSets[current]},
        {}
    );
    cmd.pushConstants(
        *this->pipelineLayout,
        vk::ShaderStageFlagBits::eCompute,
        0,
        vk::ArrayProxy<const uint32_t>{golSize, golSize}
    );
    const vk::MemoryBarrier2 beforeCompute{
        .srcStageMask =
            vk::PipelineStageFlagBits2::eTransfer |
            vk::PipelineStageFlagBits2::eComputeShader,
        .srcAccessMask =
            vk::AccessFlagBits2::eTransferRead |
            vk::AccessFlagBits2::eTransferWrite |
            vk::AccessFlagBits2::eShaderRead |
            vk::AccessFlagBits2::eShaderWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eComputeShader,
        .dstAccessMask =
            vk::AccessFlagBits2::eShaderRead |
            vk::AccessFlagBits2::eShaderWrite
    };
    cmd.pipelineBarrier2(
        vk::DependencyInfo{}.setMemoryBarriers(beforeCompute)
    );

    int groups = std::ceil(static_cast<float>(golSize) / GROUP_SIZE);
    cmd.dispatch(groups, groups, 1);

    const vk::MemoryBarrier2 beforeCopy{
        .srcStageMask = vk::PipelineStageFlagBits2::eComputeShader,
        .srcAccessMask = vk::AccessFlagBits2::eShaderWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
        .dstAccessMask = vk::AccessFlagBits2::eTransferRead
    };
    cmd.pipelineBarrier2(
        vk::DependencyInfo{}.setMemoryBarriers(beforeCopy)
    );
    transitionImageLayout(
        cmd,
        targetTexture->image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderSampledRead,
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferWrite,
        vk::ImageAspectFlagBits::eColor
    );

    cmd.copyBufferToImage(
        dstBuf,
        targetTexture->image,
        vk::ImageLayout::eTransferDstOptimal,
        {vk::BufferImageCopy{
            .imageSubresource = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .layerCount = 1
            },
            .imageExtent = {golSize, golSize, 1}
        }}
    );
    transitionImageLayout(
        cmd,
        targetTexture->image,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferWrite,
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderSampledRead,
        vk::ImageAspectFlagBits::eColor
    );

    this->current ^= 1;
}

void GOLPass::initGOLData(const VulkanContext& ctx, std::span<const int8_t> data) {
    if (data.size() != static_cast<size_t>(golSize) * golSize) {
        throw std::runtime_error("GOL Data Size Does NOT Match");
    }
    this->golData[0]->loadSync({(const uint8_t*)data.data(), data.size()}, ctx);
}
