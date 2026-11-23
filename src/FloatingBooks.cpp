// FloatingBooks.cpp
#include "FloatingBooks.h"
#include <cmath>

bool FloatingBooks::load() {
    // Seven different closed books. Each is scaled so its longest side is
    // 1.3 units, so models made at very different scales match.
    const char* files[] = {
        "models/book.glb",
        "models/brimstone_boogie_alchemy_book.glb",
        "models/devils_bible_-_dark_fantasy_book.glb",
        "models/old_book.glb",
        "models/wyrm_hunter_s_guide.glb",
        "models/de_stirpium.glb",          // very detailed: used once
        "models/coptic_prayer_book.glb",   // very detailed: used once
    };
    bool ok = true;
    models.resize(7);
    for (int i = 0; i < 7; ++i) ok = models[i].load(files[i], 1.3f) && ok;

    // 12 slots on the spiral. The 5 lighter models are used twice.
    const int order[12] = {0, 1, 2, 3, 4, 5, 6, 0, 1, 2, 3, 4};
    for (int i = 0; i < 12; ++i) {
        FloatingBook b;
        b.modelIndex = order[i];
        b.angle = i * glm::radians(50.0f);      // 50 degrees further round each time
        b.height = 2.4f + i * 0.65f;            // and 0.65 units higher -> a helix
        b.spinSpeed = (i % 2 == 0) ? 0.9f : -0.9f;
        b.phase = i * 0.7f;
        books.push_back(b);
    }
    return ok;
}

void FloatingBooks::update(float deltaTime, const Settings& s) {
    // Each clock pauses (not resets) when its switch is off, so nothing jumps.
    if (s.parentOn) parentTime += deltaTime;
    if (s.childOn) childTime += deltaTime;

    // PARENT: float up/down and turn around the vertical axis.
    parentMatrix = glm::translate(glm::mat4(1.0f),
        glm::vec3(0.0f, 0.6f * std::sin(parentTime * 0.8f), 0.0f));
    parentMatrix = glm::rotate(parentMatrix, parentTime * 0.35f,
                               glm::vec3(0.0f, 1.0f, 0.0f));
}

void FloatingBooks::draw(const Renderer& r) const {
    // The parent's own geometry: a floating mirror-like crystal pillar.
    // It is drawn with parentMatrix only, so it shows the parent's motion.
    SurfaceMaterial crystal;
    crystal.color = glm::vec3(0.10f, 0.09f, 0.14f);   // dark glass
    crystal.shininess = 128.0f;
    crystal.specular = 1.0f;
    crystal.reflectivity = 0.9f;     // reflects the sky cubemap
    crystal.emissive = 0.0f;
    glm::mat4 pillar = glm::translate(parentMatrix, {0, 7.5f, 0});
    pillar = glm::rotate(pillar, glm::radians(45.0f), glm::vec3(0, 1, 0));
    r.box(glm::scale(pillar, {0.7f, 9.0f, 0.7f}), crystal);
    // Two glowing caps on the crystal.
    SurfaceMaterial cap;
    cap.color = glm::vec3(1.0f, 0.7f, 0.35f);
    cap.emissive = 2.5f;
    cap.castsShadow = false;
    for (float y : {3.0f, 12.0f}) {
        glm::mat4 m = glm::translate(parentMatrix, {0, y, 0});
        m = glm::rotate(m, glm::radians(45.0f), glm::vec3(1, 0, 1));
        r.box(glm::scale(m, glm::vec3(0.45f)), cap);
    }

    // CHILDREN
    const float radius = 3.5f;
    for (const FloatingBook& b : books) {
        glm::vec3 spiralPos(radius * std::cos(b.angle), b.height,
                            radius * std::sin(b.angle));

        // The child's own animation.
        float bob = 0.3f * std::sin(childTime * 2.0f + b.phase);
        float spin = childTime * b.spinSpeed;
        float tilt = 0.35f * std::sin(childTime * 1.5f + b.phase);

        glm::mat4 local(1.0f);
        local = glm::translate(local, spiralPos + glm::vec3(0, bob, 0));
        local = glm::rotate(local, spin, glm::vec3(0, 1, 0));
        local = glm::rotate(local, tilt, glm::vec3(1, 0, 0));

        // FINAL = PARENT * CHILD (the model's baseMatrix is applied in Model::draw)
        r.model(models[b.modelIndex], parentMatrix * local);
    }
}
