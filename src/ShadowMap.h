// ShadowMap.h
// ADVANCED FEATURE: shadow mapping for the moonlight.
// Pass 1 renders the scene's depth as seen FROM THE MOON into a texture.
// Pass 2 (scene.frag) compares each pixel's distance to the moon with that
// texture: if something nearer to the moon is in the way, the pixel is in shadow.
#pragma once

#include "GLCommon.h"

class ShadowMap {
public:
    int resolution = 2048;
    GLuint fbo = 0;
    GLuint depthTexture = 0;
    glm::mat4 lightSpace = glm::mat4(1.0f);   // moon's projection * view

    void create();
    // Builds the moon's camera: orthographic (parallel rays, like sunlight)
    // looking at the room centre from far away along 'towardsMoon'.
    void updateLight(const glm::vec3& towardsMoon);
    void begin();   // bind the shadow framebuffer, clear depth
    void end(int screenWidth, int screenHeight);
    void destroy();
};
