#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <glm/glm.hpp>

class Model;

struct Object {
    Model& model;
    glm::fvec3 position;
    glm::fvec3 scale;
    // rotation
    float angle;
    glm::vec3 axis;

    glm::fmat4x4 calcModelMatrix();
};

#endif  // OBJECT_HPP
