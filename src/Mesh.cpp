// Mesh.cpp
#include "Mesh.h"
#include <cstddef>   // offsetof
#include <cmath>

void Mesh::upload(const std::vector<Vertex>& vertices,
                  const std::vector<unsigned int>& indices) {
    indexCount = static_cast<GLsizei>(indices.size());

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);

    // attribute index, number of floats, where it starts inside one Vertex
    auto attribute = [](GLuint index, GLint count, size_t offset) {
        glVertexAttribPointer(index, count, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<void*>(offset));
        glEnableVertexAttribArray(index);
    };
    attribute(0, 3, offsetof(Vertex, position));
    attribute(1, 3, offsetof(Vertex, normal));
    attribute(2, 2, offsetof(Vertex, uv));
    attribute(3, 4, offsetof(Vertex, tangent));

    glBindVertexArray(0);
}

void Mesh::draw() const {
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
}

void Mesh::destroy() {
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);
}

Mesh createCube() {
    // The six face normals.
    const glm::vec3 normals[6] = {
        { 0, 0, 1}, { 0, 0,-1}, { 1, 0, 0},
        {-1, 0, 0}, { 0, 1, 0}, { 0,-1, 0}
    };

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (const glm::vec3& n : normals) {
        // Two directions lying flat on the face: a = +u direction, b = +v.
        glm::vec3 a = (std::abs(n.y) > 0.5f) ? glm::vec3(1, 0, 0)
                                              : glm::vec3(0, 1, 0);
        glm::vec3 b = glm::cross(n, a);      // so b = cross(N, T): w = +1

        unsigned int start = static_cast<unsigned int>(vertices.size());
        glm::vec3 c = n * 0.5f;              // centre of the face
        vertices.push_back({c - a * 0.5f - b * 0.5f, n, {0, 0}, glm::vec4(a, 1)});
        vertices.push_back({c + a * 0.5f - b * 0.5f, n, {1, 0}, glm::vec4(a, 1)});
        vertices.push_back({c + a * 0.5f + b * 0.5f, n, {1, 1}, glm::vec4(a, 1)});
        vertices.push_back({c - a * 0.5f + b * 0.5f, n, {0, 1}, glm::vec4(a, 1)});

        // Two triangles per face. Winding is not important (no culling).
        const unsigned int quad[6] = {0, 1, 2, 0, 2, 3};
        for (unsigned int q : quad) indices.push_back(start + q);
    }

    Mesh cube;
    cube.upload(vertices, indices);
    return cube;
}
