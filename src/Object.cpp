#include "Object.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

glm::fmat4x4 Object::calcModelMatrix() {
    glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 r = glm::mat4_cast(glm::angleAxis(angle, glm::normalize(axis)));
    glm::mat4 s = glm::scale(glm::mat4(1.f), scale);

    return t * r * s;
}
