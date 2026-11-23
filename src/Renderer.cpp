// Renderer.cpp
#include "Renderer.h"

void Renderer::box(const glm::mat4& model, const SurfaceMaterial& m) const {
    if (depthPass) {
        // Shadow pass: only objects that cast shadows, only the matrix.
        if (!m.castsShadow) return;
        shader->setMat4("model", model);
        cube->draw();
        return;
    }

    shader->setMat4("model", model);
    shader->setInt("mapping", m.mapping);
    shader->setVec2("uvScale", m.uvScale);
    shader->setVec3("baseColor", m.color);
    shader->setFloat("shininess", m.shininess);
    shader->setFloat("specularStrength", m.specular);
    shader->setFloat("reflectivity", m.reflectivity);
    shader->setFloat("emissive", m.emissive);
    shader->setFloat("normalStrength", m.normalStrength);
    shader->setInt("hasAlbedoMap", m.albedo ? 1 : 0);
    shader->setInt("hasNormalMap", m.normal ? 1 : 0);
    shader->setInt("hasOrmMap", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m.albedo ? m.albedo : white);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m.normal ? m.normal : flatNormal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, defaultOrm);
    cube->draw();
}

void Renderer::model(const Model& mdl, const glm::mat4& world, float shininess,
                     float specular) const {
    if (depthPass) {
        mdl.drawDepth(*shader, world);
        return;
    }
    shader->setInt("mapping", MAP_MESH_UV);
    shader->setVec2("uvScale", glm::vec2(1.0f));
    shader->setVec3("baseColor", glm::vec3(1.0f));
    shader->setFloat("shininess", shininess);
    shader->setFloat("specularStrength", specular);
    shader->setFloat("reflectivity", 0.0f);
    shader->setFloat("emissive", 0.0f);
    shader->setFloat("normalStrength", 1.0f);
    shader->setInt("hasAlbedoMap", 1);
    mdl.draw(*shader, world);   // binds the model's own 3 textures
}
