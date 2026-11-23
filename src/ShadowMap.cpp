// ShadowMap.cpp
#include "ShadowMap.h"

void ShadowMap::create() {
    // A depth-only texture: each texel stores "distance to the nearest
    // surface" from the light's point of view.
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, resolution, resolution, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // Outside the map counts as "lit" (depth 1.0 = far away).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float border[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

    // Framebuffer with only a depth attachment (no colour needed).
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           depthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMap::updateLight(const glm::vec3& towardsMoon) {
    glm::vec3 target(0.0f, 4.0f, 0.0f);
    glm::vec3 eye = target + glm::normalize(towardsMoon) * 30.0f;
    // A box 32 x 32 units wide around the room, 1..70 units deep.
    glm::mat4 projection = glm::ortho(-16.0f, 16.0f, -16.0f, 16.0f, 1.0f, 70.0f);
    glm::mat4 view = glm::lookAt(eye, target, glm::vec3(0.0f, 0.0f, -1.0f));
    lightSpace = projection * view;
}

void ShadowMap::begin() {
    glViewport(0, 0, resolution, resolution);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::end(int screenWidth, int screenHeight) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
}

void ShadowMap::destroy() {
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &depthTexture);
}
