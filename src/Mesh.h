// Mesh.h
// A Mesh is triangles stored on the GPU: one VBO (vertex data), one EBO
// (triangle indices) and one VAO (the recipe that says how to read the VBO).
#pragma once

#include "GLCommon.h"
#include <vector>

// One corner of a triangle. The layout numbers (0-3) match the
// "layout(location = N)" lines in shaders/scene.vert.
struct Vertex {
    glm::vec3 position;  // location 0: where the corner is (object space)
    glm::vec3 normal;    // location 1: which way the surface faces (lighting)
    glm::vec2 uv;        // location 2: where on the texture this corner sits
    glm::vec4 tangent;   // location 3: xyz = direction of +u on the surface,
                         //             w = +1/-1 so bitangent = cross(N,T) * w
                         //             (needed for normal mapping)
};

struct Mesh {
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei indexCount = 0;

    // Copy vertices + indices to the GPU and describe the vertex layout.
    void upload(const std::vector<Vertex>& vertices,
                const std::vector<unsigned int>& indices);
    void draw() const;
    void destroy();
};

// A unit cube from -0.5 to +0.5. Scaled/moved by a model matrix it becomes
// walls, shelves, pillars, candles ... (the whole room is this one mesh).
Mesh createCube();
