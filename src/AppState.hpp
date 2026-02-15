#ifndef APPSTATE_HPP
#define APPSTATE_HPP
#include "utils.hpp"

struct AppState {
    RGBAColor clearColor{0.45f, 0.53f, 0.65f, 1.f};
    bool showDemoWindow = false;
    uint64_t lastRenderTimestamp;
    float frameTime;
};

#endif  // APPsTATE_HPP
