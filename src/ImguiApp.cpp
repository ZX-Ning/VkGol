#include "ImguiApp.hpp"

#include <backends/imgui_impl_glfw.h>
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

ImDrawData* ImguiApp::drawImgui(AppState& state, OptRef<Scene> scene) {
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();

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
        if (scene.has_value()) {
            Scene& sceneRef = *scene;
            ImGui::DragFloat3(
                "Front",
                reinterpret_cast<float*>(&sceneRef.view.front),
                0.01f,
                -20.f,
                10.f
            );

            float degreeFov = radianToDegree(sceneRef.camera.fovy);
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
            sceneRef.camera.fovy = degreeToRadian(degreeFov);



            auto& eye = sceneRef.view.eye;
            ImGui::InputFloat3(
                "Coord",
                reinterpret_cast<float*>(&eye),
                "%.2f"
            );
            if (ImGui::Button("Reset Camera")) {
                sceneRef.resetCamera();
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

void ImguiApp::renderVk(ImDrawData* drawData, vk::raii::CommandBuffer& cmd) {
    ImGui_ImplVulkan_RenderDrawData(drawData, *cmd);
    ;
}

ImguiApp::ImguiApp(WindowApp& window, VulkanContext& context, SwapChain& swapChain) {
    initForGlfw(window.getWindowPtr());
    initImguiForVk(window, context, swapChain.surfaceFormat.format, swapChain.images.size());
}

void ImguiApp::initForGlfw(GLFWwindow* window) {
    ImGui::CreateContext();
    if (!ImGui_ImplGlfw_InitForVulkan(window, true)) {
        throw std::runtime_error("Failed to init IMGUI for GLFW");
    }
}

ImguiApp::~ImguiApp() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
