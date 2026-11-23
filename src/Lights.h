// Lights.h
// LIGHTING: one directional moonlight + point lights (12 candles and the
// magic light that floats with the spiral).
#pragma once

#include "Shader.h"
#include "Settings.h"
#include <vector>

struct Lights {
    glm::vec3 ambient = glm::vec3(0.0f);
    glm::vec3 moonDirection = glm::normalize(glm::vec3(-0.35f, 1.0f, -0.55f));
    glm::vec3 moonColor = glm::vec3(0.0f);

    std::vector<glm::vec3> pointPos;     // where each point light is
    std::vector<glm::vec3> pointColor;   // colour * brightness
    std::vector<float> candleFlicker;    // 0.7 .. 1.0 per candle (also sizes the flame)

    // Recompute colours/positions for this frame (flicker, switches).
    void update(float time, const glm::mat4& parentMatrix, const Settings& s);
    // Send everything to scene.frag.
    void upload(const Shader& shader) const;
};

// Where candle k (0-11) stands: on the pillar between wall segments.
glm::vec3 candlePosition(int k);
