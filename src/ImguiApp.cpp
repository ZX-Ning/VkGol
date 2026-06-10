#include "ImguiApp.hpp"

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

#include <vulkan/vulkan_raii.hpp>

#include "AppState.hpp"
#include "Consts.hpp"
#include "Scene.hpp"
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

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoInputs;
    {
        ImGui::Begin("##Fullscreen", nullptr, window_flags);
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        float w = viewport->Size.x / 2.f;
        float h = viewport->Size.y / 2.f;
        drawList->AddLine({w, h - 8.f}, {w, h + 8.f}, 0xFFFFFFFF, 2.f);
        drawList->AddLine({w - 10.f, h}, {w + 10.f, h}, 0xFFFFFFFF, 2.f);
        ImGui::End();
    }

    // auto imguiScale = ImGui::GetIO().DisplayFramebufferScale;
    // auto size = ImGui::GetMainViewport()->Size;
    // std::println("Imgui scale: {}x{}", imguiScale.x, imguiScale.y);
    // ImGui::SetNextWindowSize(ImVec2(size.x * 0.75, size.y * 0.15), ImGuiCond_Appearing);
    if (state.showImGui) {
        ImGui::Begin("Hello, world!");
        ImGui::ColorEdit3(
            "clear color",
            state.clearColor.getRaw()
        );
        // ImGui::SameLine();
        // ImGui::Checkbox("Demo Window", &state.showDemoWindow);
        if (state.currentScene.has_value()) {
            Scene& scene = *state.currentScene;
            ImGui::DragFloat(
                "Move Speed (1/s)",
                reinterpret_cast<float*>(&state.moveSpeed),
                0.01f,
                0.f,
                20.f,
                "%.2f"
            );
            ImGui::DragFloat(
                "Mouse (deg/px)",
                reinterpret_cast<float*>(&state.mouseSpeed),
                0.002f,
                0.f,
                1.f,
                "%.5f"
            );
            View view = (*state.currentScene).get().view;
            ImGui::Text("View Front: %.1f, %.1f, %.1f", view.front[0], view.front[1], view.front[2]);
            ImGui::Text("View Up: %.1f, %.1f, %.1f", view.up[0], view.up[1], view.up[2]);

            float degreeFov = radianToDegree(scene.camera.fovy);
            ImGui::DragFloat(
                "Fov",
                &degreeFov,
                0.05f,
                -360.f,
                360.f,
                "%.1f"
            );
            ImGui::DragFloat(
                "Rotate Speed",
                &state.rotationSpeed,
                1.f,
                0.f,
                720.f,
                "%.2f"
            );
            scene.camera.fovy = degreeToRadian(degreeFov);

            auto& eye = scene.view.eye;
            ImGui::InputFloat3(
                "Coord",
                reinterpret_cast<float*>(&eye),
                "%.2f"
            );
            if (ImGui::Button("Reset Camera")) {
                scene.resetCamera();
            }
        }
        if (getTimestampMs() - lastShowFpsTime > (float)FPS_UPDATE_INTERVAL_MS) {
            lastframeTime = state.frameTime;
            lastShowFpsTime = getTimestampMs();
        }
        ImGui::Text("Frame time: %.2f ms (%.0f fps)", lastframeTime, 1000.f / lastframeTime);
        if (ImGui::Button("Quit")) {
            state.quit = true;
        }
        ImGui::End();
    }
    // if (state.showDemoWindow) {
    //     ImGui::ShowDemoWindow(&(state.showDemoWindow));
    // }
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
