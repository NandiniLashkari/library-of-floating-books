// TextOverlay.cpp
#include "TextOverlay.h"
#include "Texture.h"
#include <vector>

// The atlas: 16 columns x 6 rows, characters 32 (space) to 127.
static const int ATLAS_COLS = 16;
static const int ATLAS_ROWS = 6;

bool TextOverlay::create(const std::string& fontAtlasPath) {
    bool ok = shader.load("shaders/text.vert", "shaders/text.frag");
    atlas = Texture::load2D(fontAtlasPath, Texture::Kind::Data);
    // Letters should not blur into each other: no repeat at the edges.
    glBindTexture(GL_TEXTURE_2D, atlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // A dynamic buffer we refill for every string: x, y, u, v per vertex.
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    return ok && atlas != 0;
}

void TextOverlay::begin(int screenWidth, int screenHeight) {
    screen = glm::vec2(screenWidth, screenHeight);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    // The atlas is premultiplied by ImageIO, but text.frag uses only alpha,
    // so normal "over" blending is right here.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    shader.use();
    shader.setVec2("screenSize", screen);
    shader.setInt("fontAtlas", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlas);
}

void TextOverlay::end() {
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void TextOverlay::drawQuads(const float* data, int vertexCount) {
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * 4 * sizeof(float), data,
                 GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
}

void TextOverlay::panel(float x, float y, float w, float h, const glm::vec4& color) {
    shader.setInt("solid", 1);
    glUniform4fv(glGetUniformLocation(shader.id, "color"), 1, glm::value_ptr(color));
    const float q[] = {x, y, 0, 0,  x + w, y, 0, 0,  x + w, y + h, 0, 0,
                       x, y, 0, 0,  x + w, y + h, 0, 0,  x, y + h, 0, 0};
    drawQuads(q, 6);
}

void TextOverlay::text(const std::string& s, float x, float y, float size,
                       const glm::vec4& color) {
    shader.setInt("solid", 0);
    glUniform4fv(glGetUniformLocation(shader.id, "color"), 1, glm::value_ptr(color));

    std::vector<float> v;
    float w = size * 0.5f;                       // monospace: half as wide as tall
    for (char ch : s) {
        int c = static_cast<unsigned char>(ch);
        if (c < 32 || c > 127) c = '?';
        int index = c - 32;
        float u0 = float(index % ATLAS_COLS) / ATLAS_COLS;
        float v0 = float(index / ATLAS_COLS) / ATLAS_ROWS;   // row 0 = top of image
        float u1 = u0 + 1.0f / ATLAS_COLS;
        float v1 = v0 + 1.0f / ATLAS_ROWS;
        const float quad[] = {x, y, u0, v0,  x + w, y, u1, v0,  x + w, y + size, u1, v1,
                              x, y, u0, v0,  x + w, y + size, u1, v1,  x, y + size, u0, v1};
        v.insert(v.end(), quad, quad + 24);
        x += w;
    }
    if (!v.empty()) drawQuads(v.data(), static_cast<int>(v.size() / 4));
}

void TextOverlay::destroy() {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(1, &atlas);
    shader.destroy();
}
