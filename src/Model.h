// Model.h
// Loads a .glb 3D model with Assimp: its triangles AND the three textures
// stored inside the file (base colour, normal map, metallic-roughness map).
#pragma once

#include "Mesh.h"
#include "Shader.h"
#include <string>
#include <vector>

// The textures one part of a model is painted with.
struct Material {
    GLuint albedo = 0;     // base colour (sRGB)
    GLuint normal = 0;     // tangent-space normal map (data)
    GLuint orm = 0;        // glTF metallic-roughness map: G = roughness, B = metallic
    bool hasNormalMap = false;
    bool hasOrmMap = false;
};

class Model {
public:
    std::string name;
    std::vector<Mesh> meshes;
    std::vector<int> meshMaterial;      // which material each mesh uses
    std::vector<Material> materials;

    // Centres the model on (0,0,0) and scales its longest side to
    // 'targetSize' units, so models made at different scales look alike.
    glm::mat4 baseMatrix = glm::mat4(1.0f);
    glm::vec3 size = glm::vec3(1.0f);   // size after baseMatrix
    size_t triangleCount = 0;

    bool load(const std::string& path, float targetSize);

    // Binds each material's textures to units 0-2 and draws the meshes.
    // 'model' is the object's world matrix; baseMatrix is applied after it.
    void draw(const Shader& shader, const glm::mat4& model) const;

    // Shadow pass: geometry only, no textures.
    void drawDepth(const Shader& shader, const glm::mat4& model) const;
};
