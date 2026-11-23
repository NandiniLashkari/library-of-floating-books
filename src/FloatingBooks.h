// FloatingBooks.h
// HIERARCHICAL ANIMATION (from Deliverable 2, now with 7 different models).
//   Parent = the whole spiral: turns around Y and floats up and down.
//   Children = 12 books: each bobs, spins and tilts on its own.
//   Final matrix of a book = parentMatrix * local * model.baseMatrix
#pragma once

#include "Renderer.h"
#include "Settings.h"
#include <vector>

class FloatingBooks {
public:
    bool load();
    // Advance the two clocks (each only while its switch is on) and
    // rebuild the parent matrix.
    void update(float deltaTime, const Settings& s);
    void draw(const Renderer& r) const;

    glm::mat4 parentMatrix = glm::mat4(1.0f);

private:
    // One child of the spiral.
    struct FloatingBook {
        int modelIndex;   // which of the loaded models to draw
        float angle;      // position around the spiral (radians)
        float height;     // height on the spiral
        float spinSpeed;  // radians per second around its own axis
        float phase;      // so the books don't bob in sync
    };

    std::vector<Model> models;
    std::vector<FloatingBook> books;
    float parentTime = 0.0f;
    float childTime = 0.0f;
};
