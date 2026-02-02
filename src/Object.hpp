#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <glm/glm.hpp>

struct Model;

struct Object {
    Model& model;
    glm::fvec3 position;
    glm::fvec3 scale {1.f,1.f,1.f};
    // rotation
    float angle;
    glm::vec3 axis{0.f, 1.f, 0.f};

    glm::fmat4x4 calcModelMatrix();
};

#endif  // OBJECT_HPP
