// Renderer.h
// The scene is drawn TWICE per frame:
//   1. shadow pass - only depth, from the moon (shadow.vert/frag)
//   2. colour pass - full lighting and textures (scene.vert/frag)
// Library and FloatingBooks draw through this class, so the same drawing code
// works for both passes.
#pragma once

#include "Mesh.h"
#include "Model.h"
#include "Shader.h"

// How a surface looks. These become uniforms in scene.frag.
struct SurfaceMaterial {
    GLuint albedo = 0;            // 0 = no texture, use 'color' only
    GLuint normal = 0;            // 0 = no normal map
    int mapping = 2;              // 0 mesh UV, 1 floor, 2 box, 3 window sky, 4 ceiling
    glm::vec2 uvScale = glm::vec2(1.0f);
    glm::vec3 color = glm::vec3(1.0f);
    float shininess = 16.0f;      // Blinn-Phong exponent
    float specular = 0.15f;       // highlight strength
    float reflectivity = 0.0f;    // environment mapping amount
    float emissive = 0.0f;        // self-lit glow (HDR, can be > 1)
    float normalStrength = 1.0f;  // normal-map bump strength
    bool castsShadow = true;
};

// Texture-mapping modes (must match the constants in the shaders).
enum Mapping { MAP_MESH_UV = 0, MAP_FLOOR = 1, MAP_BOX = 2, MAP_SKY = 3, MAP_CEILING = 4 };

class Renderer {
public:
    const Shader* shader = nullptr;
    bool depthPass = false;
    const Mesh* cube = nullptr;
    GLuint flatNormal = 0;   // 1x1 (0.5,0.5,1) = "no bump"
    GLuint white = 0;        // 1x1 white
    GLuint defaultOrm = 0;   // 1x1 medium roughness, not metal

    // Draw the unit cube with this world matrix and material.
    void box(const glm::mat4& model, const SurfaceMaterial& m) const;
    // Draw an imported model using its own embedded textures.
    void model(const Model& mdl, const glm::mat4& world,
               float shininess = 32.0f, float specular = 0.35f) const;
};
