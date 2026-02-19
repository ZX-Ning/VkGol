#ifndef APPSTATE_HPP
#define APPSTATE_HPP
#include <glm/glm.hpp>
#include <unordered_map>

#include "utils.hpp"

struct Scene;

struct AppState {
    RGBAColor clearColor{0.45f, 0.53f, 0.65f, 1.f};
    uint64_t lastRenderTimestamp;
    float frameTime;
    float moveSpeed = 8.f;
    float mouseSpeed = 0.3f;

    struct InputState {
        std::unordered_map<int, int> keyState;
        glm::fvec2 mousePos;
        glm::fvec2 lastMousePos;
        int mouseState;
    } inputs;
    bool showImGui{true};
    bool quit{false};
    float rotationSpeed;
    OptRef<Scene> currentScene;
};

#endif  // APPsTATE_HPP
