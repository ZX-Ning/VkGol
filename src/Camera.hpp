#ifndef CAMERA_HPP
#define CAMERA_HPP

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
        glm::mat4 projection = glm::perspective(fovy, aspectRatio, clipNear, clipFar);
        // projection[1][1] *= -1;
        return projection;
    }
};

#endif  // CAMERA_HPP
