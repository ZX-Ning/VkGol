#include "ImguiApp.hpp"

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

#include <vulkan/vulkan_raii.hpp>

#include "AppState.hpp"
#include "Consts.hpp"
#include "WindowApp.hpp"
#include "core/Swapchain.hpp"
#include "core/VulkanContext.hpp"

void ImguiApp::initImguiForVk(
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
        16.f,
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

ImDrawData* ImguiApp::drawImgui(AppState& state) {
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();

    if (state.showImGui) {
        ImGui::Begin("Game of Life");
        ImGui::ColorEdit3("Background", state.clearColor.data());
        int newGolSize = state.golSize;
        if (ImGui::SliderInt("GOL Size", &newGolSize, 16, 1<<14)) {
            if (newGolSize > state.golSize) {
                state.golSize = ((newGolSize + 15) / 16) * 16;
            }
            else if (newGolSize < state.golSize) {
                state.golSize = (newGolSize / 16) * 16;
            }
        }
        ImGui::Text("Grid: %d x %d", state.golSize, state.golSize);
        if (ImGui::Button("Reset")) {
            state.resetGOLRequested = true;
        }
        ImGui::End();
    }

    ImGui::Render();
    return ImGui::GetDrawData();
}

bool ImguiApp::wantMouse() {
    return ImGui::GetIO().WantCaptureMouse;
}

void ImguiApp::renderVk(ImDrawData* drawData, vk::raii::CommandBuffer& cmd) {
    ImGui_ImplVulkan_RenderDrawData(drawData, *cmd);
    ;
}

ImguiApp::ImguiApp(WindowApp& window, VulkanContext& context, SwapChain& swapChain) {
    initForSdl(window);
    initImguiForVk(window, context, swapChain.surfaceFormat.format, swapChain.images.size());
}

void ImguiApp::initForSdl(WindowApp& windowApp) {
    ImGui::CreateContext();
    if (!ImGui_ImplSDL3_InitForVulkan(windowApp.getWindowPtr())) {
        throw std::runtime_error("Failed to init IMGUI for SDL3");
    }
    windowApp.eventCallback = [](const SDL_Event& event) {
        ImGui_ImplSDL3_ProcessEvent(&event);
    };
}

ImguiApp::~ImguiApp() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}
