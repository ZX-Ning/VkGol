// imgui
#ifndef IMGUI_HPP
#define IMGUI_HPP

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

#include <vulkan/vulkan_raii.hpp>

#include "AppState.hpp"
#include "Consts.hpp"
#include "Scene.hpp"
#include "WindowApp.hpp"
#include "core/VulkanContext.hpp"

namespace Imgui {
inline void initImguiForVk(
    WindowApp& windowApp,
    const VulkanContext& context,
    vk::Format format,
    uint32_t imageCount
) {
    ImGuiIO* imguiIo = &ImGui::GetIO();
    imguiIo->ConfigFlags |= ImGuiConfigFlags_IsSRGB;
    // imguiIo->IniFilename = NULL;

    // font
    ImFontConfig fontcfg;
    fontcfg.OversampleH = 2;
    fontcfg.OversampleV = 2;
    fontcfg.PixelSnapH = true;
    fontcfg.PixelSnapV = true;
    fontcfg.RasterizerDensity = 0.86f;
    imguiIo->Fonts->AddFontFromFileTTF(
        Consts::FONT,
        17.f,
        &fontcfg
    );
    ImGuiStyle* style = &ImGui::GetStyle();

    // set style
    ImGui::StyleColorsDark();
    style->WindowRounding = 5.f;
    style->FrameRounding = 5.f;
    style->WindowPadding = {8, 5};
    style->FramePadding = {3, 2};
    auto [scaleType, scale] = windowApp.getScale();
    if (scaleType == WindowApp::WINDOWS_OR_X11) {
        style->FontScaleDpi = scale;
        style->ScaleAllSizes(scale);
    }

    vk::PipelineRenderingCreateInfoKHR pipelineInfo{
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &format,
        .depthAttachmentFormat = vk::Format::eD32Sfloat
    };

    auto vert = readFile(Consts::SHADER_IMGUI_VERT_PATH);
    vk::ShaderModuleCreateInfo vertInfo{
        .codeSize = getVectorSize(vert),
        .pCode = reinterpret_cast<uint32_t*>(vert.data())
    };
    auto frag = readFile(Consts::SHADER_IMGUI_FRAG_PATH);
    vk::ShaderModuleCreateInfo fragInfo{
        .codeSize = getVectorSize(frag),
        .pCode = reinterpret_cast<uint32_t*>(frag.data())
    };

    ImGui_ImplVulkan_InitInfo initInfo{
        .ApiVersion = VK_API_VERSION_1_3,
        .Instance = *context.instance,
        .PhysicalDevice = *context.physicalDevice,
        .Device = *context.device,
        .QueueFamily = context.queueFamilyIndex,
        .Queue = *context.queue,
        .DescriptorPoolSize = 0xff,
        .MinImageCount = imageCount,
        .ImageCount = imageCount,
        .PipelineInfoMain = {
            .PipelineRenderingCreateInfo = *pipelineInfo,
        },
        .UseDynamicRendering = true,
        .CustomShaderVertCreateInfo = vertInfo,
        .CustomShaderFragCreateInfo = fragInfo
    };

    ImGui_ImplVulkan_LoadFunctions(
        VK_API_VERSION_1_3,
        [](const char* name, void* data) {
            auto instance =
                reinterpret_cast<const vk::raii::Instance*>(data);
            return instance
                ->getDispatcher()
                ->vkGetInstanceProcAddr(**instance, name);
        },
        (void*)(&context.instance)  // using c-style cast for const + reinterpret cast
    );

    ImGui_ImplVulkan_Init(&initInfo);
}

inline ImDrawData* drawImgui(AppState& state, OptRef<Scene> scene) {
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
    {
        // auto imguiScale = ImGui::GetIO().DisplayFramebufferScale;
        // auto size = ImGui::GetMainViewport()->Size;
        // std::println("Imgui scale: {}x{}", imguiScale.x, imguiScale.y);
        // ImGui::SetNextWindowSize(ImVec2(size.x * 0.75, size.y * 0.15), ImGuiCond_Appearing);
        ImGui::Begin("Hello, world!");
        ImGui::ColorEdit3(
            "clear color",
            state.clearColor.getRaw()
        );
        // ImGui::SameLine();
        // ImGui::Checkbox("Demo Window", &state.showDemoWindow);
        if (scene.has_value()) {
            Scene& sceneRef = *scene;
            ImGui::DragFloat3(
                "Eye",
                reinterpret_cast<float*>(&sceneRef.view.eye),
                0.01f,
                -20.f,
                10.f
            );
            ImGui::DragFloat3(
                "LookAt",
                reinterpret_cast<float*>(&sceneRef.view.target),
                0.01f,
                -20.f,
                10.f
            );
            ImGui::DragFloat(
                "Rotation",
                reinterpret_cast<float*>(&sceneRef.objects.front().get().angle),
                0.01f,
                -std::numbers::pi,
                std::numbers::pi
            );
            ImGui::DragFloat(
                "Fov",
                reinterpret_cast<float*>(&sceneRef.camera.fovy),
                0.001f,
                0,
                std::numbers::pi
            );
        }

        ImGui::Text("Frame time: %.2fms", state.frameTime);
        ImGui::End();
    }
    // if (state.showDemoWindow) {
    //     ImGui::ShowDemoWindow(&(state.showDemoWindow));
    // }
    ImGui::Render();
    return ImGui::GetDrawData();
}

}  // namespace Imgui
#endif  // IMGUI_HPP
