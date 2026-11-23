// Camera.h
// First-person "fly" camera: a position plus two angles (yaw and pitch).
#pragma once

#include "GLCommon.h"

class Camera {
public:
    glm::vec3 position = glm::vec3(0.0f, 4.0f, 10.0f);
    float yaw = -90.0f;     // left / right in degrees (-90 = looking down -Z)
    float pitch = -5.0f;    // up / down in degrees

    // Direction the camera looks, from yaw and pitch (sin/cos on a sphere).
    glm::vec3 front() const;
    // World-to-camera matrix built with glm::lookAt.
    glm::mat4 viewMatrix() const;

    // Mouse movement in pixels -> turn the camera.
    void turn(float dxPixels, float dyPixels);
    // Keyboard movement for this frame (W/A/S/D/Q/E, arrows, Shift).
    void move(GLFWwindow* window, float deltaTime);
};
