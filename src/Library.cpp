// Library.cpp
#include "Library.h"
#include "Texture.h"
#include <cmath>

namespace {
const float ROOM_RADIUS = 14.0f;
const float WALL_HEIGHT = 17.0f;
const int SEGMENTS = 12;

// Rotation (degrees) that turns an object at angle 'a' around the room so
// its +Z side faces the centre.
float facingCentre(float a) {
    return glm::degrees(std::atan2(-std::cos(a), -std::sin(a)));
}

// translate * rotateY * scale, the order used for almost every box below
glm::mat4 place(const glm::mat4& parent, glm::vec3 pos, glm::vec3 size) {
    return glm::scale(glm::translate(parent, pos), size);
}
}  // namespace

bool Library::load() {
    wood        = Texture::load2D("models/textures/floor_wood.png",   Texture::Kind::Color);
    stone       = Texture::load2D("models/textures/stone_albedo.png", Texture::Kind::Color);
    stoneNormal = Texture::load2D("models/textures/stone_normal.png", Texture::Kind::Data);
    leather     = Texture::load2D("models/textures/leather.png",      Texture::Kind::Color);
    rug         = Texture::load2D("models/textures/rug.png",          Texture::Kind::Color);
    // The rug is placed once (a decal), so it must not repeat at its edges.
    glBindTexture(GL_TEXTURE_2D, rug);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    const std::string faces[6] = {
        "models/textures/sky_px.png", "models/textures/sky_nx.png",
        "models/textures/sky_py.png", "models/textures/sky_ny.png",
        "models/textures/sky_pz.png", "models/textures/sky_nz.png"};
    sky = Texture::loadCubemap(faces);

    bool ok = openBookStand.load("models/ancient_book_made_with_ai.glb", 2.2f);
    ok = popUpBook.load("models/book-case.glb", 1.7f) && ok;
    return ok && wood && stone && rug && sky;
}

void Library::setFiltering(int mode) {
    for (GLuint t : {wood, stone, stoneNormal, leather, rug}) Texture::setFiltering(t, mode);
}

void Library::drawBookshelf(const Renderer& r, const glm::mat4& shelf, int seed) const {
    const float width = 5.0f, height = 8.0f, depth = 1.0f;

    SurfaceMaterial frame;
    frame.albedo = wood;
    frame.uvScale = glm::vec2(0.45f);
    frame.color = glm::vec3(0.62f, 0.45f, 0.36f);   // darker, stained wood
    frame.shininess = 24.0f;
    frame.specular = 0.25f;

    // Frame: back, two sides, top.
    r.box(place(shelf, {0, height / 2, -depth / 2 + 0.05f}, {width, height, 0.1f}), frame);
    r.box(place(shelf, {-width / 2, height / 2, 0}, {0.2f, height, depth}), frame);
    r.box(place(shelf, { width / 2, height / 2, 0}, {0.2f, height, depth}), frame);
    r.box(place(shelf, {0, height, 0}, {width + 0.2f, 0.2f, depth}), frame);

    // Book spines: the same leather texture, tinted a different colour each.
    const glm::vec3 tints[6] = {
        {0.42f, 0.14f, 0.11f}, {0.16f, 0.20f, 0.30f}, {0.20f, 0.27f, 0.17f},
        {0.48f, 0.38f, 0.22f}, {0.30f, 0.17f, 0.12f}, {0.26f, 0.17f, 0.27f}};
    SurfaceMaterial spine;
    spine.albedo = leather;
    spine.uvScale = glm::vec2(1.5f);
    spine.shininess = 40.0f;
    spine.specular = 0.3f;
    spine.castsShadow = false;      // too small to matter, saves time

    for (int level = 0; level < 5; ++level) {
        float y = 0.1f + level * 1.6f;
        r.box(place(shelf, {0, y, 0}, {width, 0.15f, depth}), frame);

        float x = -width / 2 + 0.25f;
        int i = 0;
        while (x < width / 2 - 0.4f) {
            // Repeatable pseudo-random sizes: same result every frame.
            int h = seed * 131 + level * 31 + i * 17 + 1000;
            float w = 0.18f + 0.08f * ((h * 13) % 5) / 4.0f;
            float bookH = 0.9f + 0.4f * ((h * 7) % 6) / 5.0f;
            spine.color = tints[h % 6];
            r.box(place(shelf, {x + w / 2, y + 0.075f + bookH / 2, 0.05f},
                        {w * 0.92f, bookH, 0.7f}), spine);
            x += w;
            ++i;
        }
    }
}

void Library::drawWindow(const Renderer& r, const glm::mat4& seg) const {
    SurfaceMaterial skyPanel;          // shows the sky cubemap (mapping 3)
    skyPanel.mapping = MAP_SKY;
    skyPanel.castsShadow = false;

    SurfaceMaterial frame;
    frame.albedo = wood;
    frame.color = glm::vec3(0.35f, 0.25f, 0.2f);
    frame.uvScale = glm::vec2(1.0f);

    SurfaceMaterial sill;
    sill.albedo = stone;
    sill.normal = stoneNormal;
    sill.normalStrength = 2.5f;
    sill.uvScale = glm::vec2(0.5f);

    // Rectangle + three narrowing steps = a pointed arch.
    r.box(place(seg, {0, 7.0f, 0.2f}, {3.2f, 10.0f, 0.1f}), skyPanel);
    r.box(place(seg, {0, 12.4f, 0.2f}, {2.6f, 0.8f, 0.1f}), skyPanel);
    r.box(place(seg, {0, 13.1f, 0.2f}, {1.8f, 0.6f, 0.1f}), skyPanel);
    r.box(place(seg, {0, 13.6f, 0.2f}, {0.9f, 0.5f, 0.1f}), skyPanel);
    // Window bars and the stone sill.
    r.box(place(seg, {0, 8.0f, 0.3f}, {0.12f, 11.5f, 0.12f}), frame);
    for (float y : {4.5f, 8.0f, 11.5f})
        r.box(place(seg, {0, y, 0.3f}, {3.2f, 0.12f, 0.12f}), frame);
    r.box(place(seg, {0, 1.9f, 0.5f}, {3.8f, 0.25f, 0.8f}), sill);
}

void Library::draw(const Renderer& r, const Lights& lights, float time) const {
    // ---------------- floor and ceiling ----------------
    SurfaceMaterial floorMat;
    floorMat.albedo = wood;
    floorMat.mapping = MAP_FLOOR;          // planar mapping from world x,z
    floorMat.uvScale = glm::vec2(0.25f);   // one texture copy per 4 x 4 units
    floorMat.shininess = 48.0f;
    floorMat.specular = 0.45f;             // polished floor: visible highlights
    floorMat.castsShadow = false;
    r.box(place(glm::mat4(1.0f), {0, -0.1f, 0}, {32, 0.2f, 32}), floorMat);

    SurfaceMaterial ceilingMat = floorMat;
    ceilingMat.mapping = MAP_CEILING;      // wood + a round opening to the sky
    ceilingMat.color = glm::vec3(0.45f, 0.35f, 0.3f);
    ceilingMat.specular = 0.1f;
    r.box(place(glm::mat4(1.0f), {0, WALL_HEIGHT, 0}, {32, 0.2f, 32}), ceilingMat);

    // ---------------- materials reused around the room ----------------
    SurfaceMaterial wallMat;
    wallMat.albedo = stone;
    wallMat.normal = stoneNormal;          // normal mapping: bumpy bricks
    wallMat.normalStrength = 2.5f;
    wallMat.uvScale = glm::vec2(0.35f);
    wallMat.shininess = 12.0f;
    wallMat.specular = 0.12f;
    wallMat.castsShadow = false;

    SurfaceMaterial pillarMat = wallMat;
    pillarMat.color = glm::vec3(1.1f);
    pillarMat.castsShadow = true;

    SurfaceMaterial balcony;
    balcony.albedo = wood;
    balcony.uvScale = glm::vec2(0.5f);
    balcony.color = glm::vec3(0.5f, 0.38f, 0.3f);
    balcony.shininess = 30.0f;
    balcony.specular = 0.3f;

    SurfaceMaterial candleWax;
    candleWax.color = glm::vec3(0.95f, 0.88f, 0.72f);
    candleWax.emissive = 0.6f;             // wax glows a little from the flame
    SurfaceMaterial flame;
    flame.color = glm::vec3(1.0f, 0.62f, 0.25f);
    flame.emissive = 9.0f;                 // much brighter than 1.0 -> bloom
    flame.castsShadow = false;

    const float segWidth = 2.0f * ROOM_RADIUS * std::tan(glm::radians(15.0f));

    for (int k = 0; k < SEGMENTS; ++k) {
        float a = glm::radians(k * 30.0f);
        glm::vec3 dir(std::cos(a), 0.0f, std::sin(a));

        // This segment's own space: origin on the wall, +Z towards the centre.
        glm::mat4 seg = glm::translate(glm::mat4(1.0f), dir * ROOM_RADIUS);
        seg = glm::rotate(seg, glm::radians(facingCentre(a)), glm::vec3(0, 1, 0));

        r.box(place(seg, {0, WALL_HEIGHT / 2, -0.2f}, {segWidth + 0.4f, WALL_HEIGHT, 0.4f}),
              wallMat);

        bool isWindow = (k == 8 || k == 9 || k == 10);   // back of the room
        if (isWindow) {
            drawWindow(r, seg);
        } else {
            glm::mat4 lower = glm::translate(seg, {0, 0, 0.6f});
            drawBookshelf(r, lower, k);
            drawBookshelf(r, glm::translate(lower, {0, 8.6f, 0}), k + 50);
            r.box(place(seg, {0, 8.35f, 1.0f}, {segWidth, 0.3f, 1.6f}), balcony);
            r.box(place(seg, {0, 9.2f, 1.75f}, {segWidth, 0.1f, 0.1f}), balcony);
        }

        // Pillar + candle between this segment and the next.
        float b = a + glm::radians(15.0f);
        glm::mat4 pil = glm::translate(glm::mat4(1.0f),
            glm::vec3(std::cos(b), 0, std::sin(b)) * (ROOM_RADIUS - 0.5f));
        pil = glm::rotate(pil, glm::radians(facingCentre(b)), glm::vec3(0, 1, 0));
        r.box(place(pil, {0, WALL_HEIGHT / 2, 0}, {1.0f, WALL_HEIGHT, 1.0f}), pillarMat);
        r.box(place(pil, {0, 0.4f, 0}, {1.4f, 0.8f, 1.4f}), pillarMat);
        r.box(place(pil, {0, 3.1f, 0.65f}, {0.5f, 0.12f, 0.5f}), balcony);
        r.box(place(pil, {0, 3.35f, 0.75f}, {0.14f, 0.4f, 0.14f}), candleWax);
        float f = lights.candleFlicker.empty() ? 1.0f : lights.candleFlicker[k];
        r.box(place(pil, {0, 3.62f, 0.75f}, {0.09f * f, 0.2f * f, 0.09f * f}), flame);
    }

    // ---------------- centre stand with an open book ----------------
    SurfaceMaterial standMat = pillarMat;
    standMat.color = glm::vec3(0.9f);
    for (float rot : {0.0f, 45.0f}) {   // two squares at 45 degrees = octagon
        glm::mat4 m = glm::rotate(glm::mat4(1.0f), glm::radians(rot), glm::vec3(0, 1, 0));
        r.box(place(m, {0, 0.15f, 0}, {2.8f, 0.3f, 2.8f}), standMat);
        r.box(place(m, {0, 0.45f, 0}, {1.6f, 0.3f, 1.6f}), standMat);
    }
    r.box(place(glm::mat4(1.0f), {0, 0.95f, 0}, {0.6f, 0.7f, 0.6f}), standMat);
    glm::mat4 lectern = glm::translate(glm::mat4(1.0f), {0, 1.45f, 0});
    lectern = glm::rotate(lectern, glm::radians(-12.0f), glm::vec3(1, 0, 0));
    r.box(place(lectern, {0, -0.05f, 0}, {2.3f, 0.12f, 1.8f}), balcony);
    r.model(openBookStand, glm::translate(lectern, {0, 0.25f, 0}), 24.0f, 0.25f);

    // ---------------- reading desk by the windows with the pop-up book ----------------
    glm::mat4 desk = glm::translate(glm::mat4(1.0f), {3.5f, 0.0f, -9.5f});
    desk = glm::rotate(desk, glm::radians(-20.0f), glm::vec3(0, 1, 0));
    r.box(place(desk, {0, 0.55f, 0}, {2.4f, 1.1f, 1.4f}), balcony);
    r.model(popUpBook, glm::translate(desk, {0, 1.1f + popUpBook.size.y / 2, 0}),
            24.0f, 0.25f);

    // ---------------- glowing dust rising around the spiral ----------------
    SurfaceMaterial dust;
    dust.color = glm::vec3(1.0f, 0.8f, 0.45f);
    dust.emissive = 3.0f;
    dust.castsShadow = false;
    for (int i = 0; i < 70; ++i) {
        float h0 = std::fmod(i * 1.37f, 1.0f);
        float radius = 1.0f + std::fmod(i * 2.71f, 6.0f);
        float speed = 0.3f + std::fmod(i * 0.53f, 0.5f);
        float y = std::fmod(h0 * 13.0f + time * speed, 13.0f) + 0.5f;
        float ang = i * 2.4f + time * 0.15f;
        float size = 0.035f + 0.02f * std::sin(time * 2.0f + i);
        r.box(place(glm::mat4(1.0f), {radius * std::cos(ang), y, radius * std::sin(ang)},
                    glm::vec3(size)), dust);
    }
}
