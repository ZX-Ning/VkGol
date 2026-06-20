#ifndef APPSTATE_HPP
#define APPSTATE_HPP
#include <glm/glm.hpp>
#include <unordered_map>

#include "utils.hpp"

struct Scene;

struct AppState {
    RGBAColor clearColor{0.13f, 0.13f, 0.13f, 1.f};
    uint64_t lastRenderTimestamp{0};

    struct InputState {
        std::unordered_map<int, int> keyState;
        glm::fvec2 mousePos;
        glm::fvec2 lastMousePos;
        int mouseState{0};
    } inputs;
    int golSize{64};
    bool resetGOLRequested{false};
    bool showImGui{true};
    bool quit{false};
};

#endif  // APPsTATE_HPP
