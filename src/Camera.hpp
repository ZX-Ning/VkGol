#ifndef CAMERA_HPP
#define CAMERA_HPP

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct View {
    glm::fvec3 eye;
    glm::fvec3 target;
    glm::fvec3 up;
    glm::fmat4x4 calcMat() {
        return glm::lookAt(eye, target, up);
    }
};

struct Camera {
    float fovy;
    float aspectRatio;
    float clipNear;
    float clipFar;
    glm::fmat4x4 clacMat() {
        return glm::perspective(fovy, aspectRatio, clipNear, clipFar);
    }
};

#endif  // CAMERA_HPP
