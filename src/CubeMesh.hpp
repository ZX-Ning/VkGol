#ifndef CUBEMESH_HPP
#define CUBEMESH_HPP

#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

#include "core/vertex.hpp"

// AI-Generagted Mesh Data
// 24 Vertices (4 per face * 6 faces)
// Positions assume a unit cube from -1.0 to 1.0
// UVs map (0,0) Top-Left to (1,1) Bottom-Right
const std::vector<SimpleVertex> CUBE_VERTICES = {
    // Front Face (+Z)
    // TL, BL, BR, TR
    {{-1.0f, 1.0f, 1.0f}, glm::fvec4{1.0f}, {0.0f, 0.0f}},
    {{-1.0f, -1.0f, 1.0f}, glm::fvec4{1.0f}, {0.0f, 1.0f}},
    {{1.0f, -1.0f, 1.0f}, glm::fvec4{1.0f}, {1.0f, 1.0f}},
    {{1.0f, 1.0f, 1.0f}, glm::fvec4{1.0f}, {1.0f, 0.0f}},

    // Back Face (-Z)
    // TL, BL, BR, TR (Relative to looking AT the face)
    {{1.0f, 1.0f, -1.0f}, glm::fvec4{1.0f}, {0.0f, 0.0f}},
    {{1.0f, -1.0f, -1.0f}, glm::fvec4{1.0f}, {0.0f, 1.0f}},
    {{-1.0f, -1.0f, -1.0f}, glm::fvec4{1.0f}, {1.0f, 1.0f}},
    {{-1.0f, 1.0f, -1.0f}, glm::fvec4{1.0f}, {1.0f, 0.0f}},

    // Left Face (-X)
    {{-1.0f, 1.0f, -1.0f}, glm::fvec4{1.0f}, {0.0f, 0.0f}},
    {{-1.0f, -1.0f, -1.0f}, glm::fvec4{1.0f}, {0.0f, 1.0f}},
    {{-1.0f, -1.0f, 1.0f}, glm::fvec4{1.0f}, {1.0f, 1.0f}},
    {{-1.0f, 1.0f, 1.0f}, glm::fvec4{1.0f}, {1.0f, 0.0f}},

    // Right Face (+X)
    {{1.0f, 1.0f, 1.0f}, glm::fvec4{1.0f}, {0.0f, 0.0f}},
    {{1.0f, -1.0f, 1.0f}, glm::fvec4{1.0f}, {0.0f, 1.0f}},
    {{1.0f, -1.0f, -1.0f}, glm::fvec4{1.0f}, {1.0f, 1.0f}},
    {{1.0f, 1.0f, -1.0f}, glm::fvec4{1.0f}, {1.0f, 0.0f}},

    // Top Face (+Y)
    {{-1.0f, 1.0f, -1.0f}, glm::fvec4{1.0f}, {0.0f, 0.0f}},
    {{-1.0f, 1.0f, 1.0f}, glm::fvec4{1.0f}, {0.0f, 1.0f}},
    {{1.0f, 1.0f, 1.0f}, glm::fvec4{1.0f}, {1.0f, 1.0f}},
    {{1.0f, 1.0f, -1.0f}, glm::fvec4{1.0f}, {1.0f, 0.0f}},

    // Bottom Face (-Y)
    {{-1.0f, -1.0f, 1.0f}, glm::fvec4{1.0f}, {0.0f, 0.0f}},
    {{-1.0f, -1.0f, -1.0f}, glm::fvec4{1.0f}, {0.0f, 1.0f}},
    {{1.0f, -1.0f, -1.0f}, glm::fvec4{1.0f}, {1.0f, 1.0f}},
    {{1.0f, -1.0f, 1.0f}, glm::fvec4{1.0f}, {1.0f, 0.0f}}
};

// 12 Triangles (2 per face)
// Winding order: CCW
const std::vector<glm::vec<3, uint32_t>> CUBE_INDICES = {
    // Front
    {0, 1, 2},
    {0, 2, 3},
    // Back
    {4, 5, 6},
    {4, 6, 7},
    // Left
    {8, 9, 10},
    {8, 10, 11},
    // Right
    {12, 13, 14},
    {12, 14, 15},
    // Top
    {16, 17, 18},
    {16, 18, 19},
    // Bottom
    {20, 21, 22},
    {20, 22, 23}
};

#endif  // CUBEMESH_HPP
