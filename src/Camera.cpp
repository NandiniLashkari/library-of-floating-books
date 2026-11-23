// Camera.cpp
#include "Camera.h"
#include <cmath>

glm::vec3 Camera::front() const {
    glm::vec3 f;
    f.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    f.y = std::sin(glm::radians(pitch));
    f.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    return glm::normalize(f);
}

glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(position, position + front(), glm::vec3(0.0f, 1.0f, 0.0f));
}

void Camera::turn(float dxPixels, float dyPixels) {
    const float sensitivity = 0.1f;
    yaw += dxPixels * sensitivity;
    // Stop at +/-89 degrees: at 90 the view direction equals "up" and
    // lookAt can no longer decide which way is sideways.
    pitch = glm::clamp(pitch - dyPixels * sensitivity, -89.0f, 89.0f);
}

void Camera::move(GLFWwindow* window, float deltaTime) {
    float speed = 5.0f * deltaTime;   // 5 units per second at any frame rate
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) speed *= 2.5f;

    glm::vec3 f = front();
    glm::vec3 right = glm::normalize(glm::cross(f, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) position += f * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) position -= f * speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) position -= right * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) position += right * speed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) position += up * speed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) position -= up * speed;

    // Arrow keys also turn (handy when recording without a mouse).
    float turnSpeed = 70.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS) yaw -= turnSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) yaw += turnSpeed;
    if (glfwGetKey(window, GLFW_KEY_UP)    == GLFW_PRESS) pitch += turnSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN)  == GLFW_PRESS) pitch -= turnSpeed;
    pitch = glm::clamp(pitch, -89.0f, 89.0f);

    // Keep the camera inside the round room.
    position.y = glm::clamp(position.y, 0.5f, 16.0f);
    glm::vec2 flat(position.x, position.z);
    if (glm::length(flat) > 11.5f) {
        flat = glm::normalize(flat) * 11.5f;
        position.x = flat.x;
        position.z = flat.y;
    }
}
