#ifndef APPSTATE_HPP
#define APPSTATE_HPP
#include <glm/glm.hpp>
#include <unordered_map>

#include "utils.hpp"

struct AppState {
    RGBAColor clearColor{0.45f, 0.53f, 0.65f, 1.f};
    uint64_t lastRenderTimestamp;
    float frameTime;
    float moveSpeed = 7.f;

    struct InputState {
        std::unordered_map<int, int> keyState;
        glm::fvec2 mousePos;
        int mouseState;
    } inputs;
    bool showImGui{true};
    bool quit{false};
    float rotationSpeed;
};

#endif  // APPsTATE_HPP
