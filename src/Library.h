// Library.h
// The static room: a 12-sided round hall built from one cube mesh.
// Each side is a two-storey bookshelf bay or an arched window; stone pillars
// with candles stand between the sides. Two open books are on display.
#pragma once

#include "Renderer.h"
#include "Lights.h"

class Library {
public:
    bool load();   // textures + the two open-book models
    // Draws everything that does not move with the hierarchy.
    void draw(const Renderer& r, const Lights& lights, float time) const;
    // Apply a texture-filtering mode (see Texture::setFiltering) to the room textures.
    void setFiltering(int mode);

    // Textures (the scene shader also needs rug + sky on fixed units).
    GLuint wood = 0, stone = 0, stoneNormal = 0, leather = 0, rug = 0, sky = 0;

private:
    Model openBookStand;     // ancient_book_made_with_ai.glb, on the centre stand
    Model popUpBook;         // book-case.glb (pop-up ship book), on a reading desk

    void drawBookshelf(const Renderer& r, const glm::mat4& shelf, int seed) const;
    void drawWindow(const Renderer& r, const glm::mat4& seg) const;
};
