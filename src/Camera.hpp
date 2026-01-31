#ifndef CAMERA_HPP
#define CAMERA_HPP

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "utils.hpp"

struct View {
    glm::fvec3 eye{0.f, 4.f, -5.f};
    glm::fvec3 target{0.f, 1.f, 0.f};
    glm::fvec3 up{0.f, 0.1f, 0.f};
    glm::fmat4x4 calcMat() {
        return glm::lookAt(eye, target, up);
    }
};

struct Camera {
    float fovy{degreeToRadian(37.8)};
    float aspectRatio{1.f};
    float clipNear{0.1f};
    float clipFar{100.f};
    glm::fmat4x4 clacMat() {
        glm::mat4 projection = glm::perspective(fovy, aspectRatio, clipNear, clipFar);
        // projection[1][1] *= -1;
        return projection;
    }
};

#endif  // CAMERA_HPP
