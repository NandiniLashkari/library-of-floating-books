// Lights.cpp
#include "Lights.h"
#include <cmath>

glm::vec3 candlePosition(int k) {
    float a = glm::radians(k * 30.0f + 15.0f);    // halfway between segments
    float r = 14.0f - 1.3f;                       // just in front of the pillar
    return glm::vec3(r * std::cos(a), 3.62f, r * std::sin(a));
}

void Lights::update(float time, const glm::mat4& parentMatrix, const Settings& s) {
    // Low ambient: the dark library is lit mostly by the lights below.
    ambient = glm::vec3(0.030f, 0.028f, 0.040f);

    // Cold blue moonlight coming down through the ceiling opening.
    moonColor = s.moonOn ? glm::vec3(0.55f, 0.68f, 1.05f) : glm::vec3(0.0f);

    pointPos.clear();
    pointColor.clear();
    candleFlicker.clear();

    // 12 candles. Flicker = two sine waves at odd speeds multiplied, which
    // looks irregular like a real flame.
    for (int k = 0; k < 12; ++k) {
        float f = 0.85f + 0.15f * std::sin(time * 13.0f + k * 2.1f)
                                * std::sin(time * 7.3f + k);
        candleFlicker.push_back(f);
        if (!s.candlesOn) continue;
        pointPos.push_back(candlePosition(k));
        pointColor.push_back(glm::vec3(1.0f, 0.55f, 0.22f) * 2.6f * f);
    }

    // Magic light in the middle of the spiral. It is attached to the parent
    // of the hierarchy, so it floats up and down with the books.
    if (s.candlesOn) {
        pointPos.push_back(glm::vec3(parentMatrix * glm::vec4(0.0f, 5.0f, 0.0f, 1.0f)));
        pointColor.push_back(glm::vec3(1.0f, 0.72f, 0.42f) * 6.0f);
    }
}

void Lights::upload(const Shader& shader) const {
    shader.setVec3("ambientColor", ambient);
    shader.setVec3("moonDirection", moonDirection);
    shader.setVec3("moonColor", moonColor);
    shader.setInt("numPointLights", static_cast<int>(pointPos.size()));
    if (!pointPos.empty()) {
        shader.setVec3Array("pointPos[0]", static_cast<int>(pointPos.size()), pointPos.data());
        shader.setVec3Array("pointColor[0]", static_cast<int>(pointColor.size()),
                            pointColor.data());
    }
}
