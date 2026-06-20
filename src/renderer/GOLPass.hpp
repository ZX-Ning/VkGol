#ifndef GOLPASS_HPP
#define GOLPASS_HPP

#include <array>
#include <cstdint>
#include <memory>
#include <span>

#include <vulkan/vulkan_raii.hpp>

struct Pipeline;
struct Texture;
struct StaticBuffer;
struct VulkanContext;

struct GOLPass {
private:
    vk::raii::PipelineLayout pipelineLayout{nullptr};
    vk::raii::DescriptorSetLayout setLayout{nullptr};
    std::array<vk::raii::DescriptorSet, 2> bindingSets{nullptr, nullptr};
    std::shared_ptr<Pipeline> updatePipe;
    std::shared_ptr<Texture> targetTexture;
    std::shared_ptr<StaticBuffer> golData[2];
    uint32_t golSize;
    int current = 0;
    bool firstUpdate = true;
    bool targetInitialized = false;

public:
    explicit GOLPass(const VulkanContext& ctx, uint32_t golSize);
    void record(vk::raii::CommandBuffer& cmd);
    void initGOLData(const VulkanContext& ctx, std::span<const int8_t> data);
    const Texture& getTexture() const {
        return *targetTexture;
    }
};

#endif // GOLPASS_HPP
