// Settings.h
// Every feature switch in one place. Keys in main.cpp flip these, the HUD
// prints them, and the renderer reads them.
#pragma once

#include <string>

struct Settings {
    // ---- Section 1: Lighting & Shading ----
    int viewMode = 0;            // [1] 0 full, 1 ambient, 2 diffuse, 3 specular, 4 normals
    bool moonOn = true;          // [2] directional moonlight
    bool candlesOn = true;       // [3] 12 candle point lights (+ magic light)
    int shadingModel = 0;        // [0] 0 Phong, 1 Gouraud, 2 Flat

    // ---- Section 2: Texture Mapping ----
    bool texturesOn = true;      // [4] image textures (off = plain clay colours)
    int filterMode = 3;          // [F] 0 nearest, 1 bilinear, 2 trilinear, 3 trilinear + anisotropic

    // ---- Section 3: Advanced Features ----
    bool normalMapOn = true;     // [5] normal mapping
    bool shadowsOn = true;       // [6] shadow mapping
    bool bloomOn = true;         // [7] HDR bloom
    bool reflectionsOn = true;   // [8] cubemap environment reflections
    bool showShadowMap = false;  // [9] show the shadow map texture

    // ---- Hierarchy (Deliverable 2) ----
    bool parentOn = true;        // [P]
    bool childOn = true;         // [C]

    // ---- On-screen labels ----
    bool showHud = true;         // [H]
    int section = 0;             // [TAB] 0 none, 1 lighting, 2 textures, 3 advanced
    std::string caption;         // short explanation of the last switch pressed
    float captionTimer = 0.0f;   // seconds left to show it
};

inline const char* shadingModelName(int m) {
    return m == 1 ? "Gouraud (per vertex)" : m == 2 ? "Flat (per face)" : "Phong (per pixel)";
}

inline const char* filterModeName(int m) {
    switch (m) {
        case 0: return "Nearest";
        case 1: return "Bilinear, no mipmaps";
        case 2: return "Trilinear mipmaps";
        default: return "Trilinear + anisotropic 8x";
    }
}

inline const char* viewModeName(int mode) {
    switch (mode) {
        case 1: return "Ambient only";
        case 2: return "Diffuse only";
        case 3: return "Specular only";
        case 4: return "Normals";
        default: return "Full Blinn-Phong";
    }
}
