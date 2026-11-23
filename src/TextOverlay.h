// TextOverlay.h
// On-screen labels (section titles and feature switches) drawn on top of the
// 3D image, so the video shows which feature is being demonstrated.
// Letters come from a bitmap-font atlas: one image with all ASCII characters
// in a 16 x 6 grid. Each letter is a small textured rectangle (2 triangles).
#pragma once

#include "Shader.h"
#include <string>

class TextOverlay {
public:
    bool create(const std::string& fontAtlasPath);
    void begin(int screenWidth, int screenHeight);   // 2D mode, blending on
    void end();                                      // back to 3D state

    // x, y = top-left corner in pixels; size = letter height in pixels.
    void text(const std::string& s, float x, float y, float size,
              const glm::vec4& color);
    void panel(float x, float y, float w, float h, const glm::vec4& color);
    float width(const std::string& s, float size) const { return s.size() * size * 0.5f; }
    void destroy();

private:
    Shader shader;
    GLuint atlas = 0, vao = 0, vbo = 0;
    glm::vec2 screen = glm::vec2(1.0f);
    void drawQuads(const float* data, int vertexCount);
};
