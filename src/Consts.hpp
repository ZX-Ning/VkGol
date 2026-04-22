#include <cstdint>

namespace Consts {

constexpr uint32_t WIDTH = 1200;
constexpr uint32_t HEIGHT = 720;
inline const char* TITLE = "Learn Vulkan";
inline const char* FONT = "assets/fonts/IBMPlex/IBMPlexSans-Regular.ttf";
inline const char* DOG_TEXTURE_PATH = "assets/textures/dog.png";
inline const char* SHADER_SPV_PATH = "shaders/shader.spv";
inline const char* SHADER_IMGUI_VERT_PATH = "shaders/imgui/vert.spv";
inline const char* SHADER_IMGUI_FRAG_PATH = "shaders/imgui/frag.spv";

inline const char* HELP_MSG = R"(
ESC: Toogle GUI
W/A/S/D: MOVE
SPACE/LEFT_SHIFT: UP/DOWN
In GUI Mode, Press left button of the mouse to move camera.
)";

};  // namespace Consts
