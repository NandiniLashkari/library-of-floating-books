// PostProcess.h
// ADVANCED FEATURE: HDR rendering + bloom + tone mapping.
//  1. The scene is drawn into a floating-point (HDR) texture, so bright
//     things like candle flames can be brighter than 1.0.
//  2. Bright pass: keep only pixels brighter than a threshold.
//  3. Blur them (Gaussian, ping-pong between two half-size textures).
//  4. Composite: scene + blurred glow -> ACES tone mapping -> gamma -> screen.
#pragma once

#include "Shader.h"

class PostProcess {
public:
    bool create(int width, int height);
    void resize(int width, int height);   // call when the window size changes

    void beginScene();                    // draw the 3D scene after this
    // Runs bright pass + blur + composite and writes to the screen.
    void finish(bool bloomOn, float exposure, bool passthrough = false);
    // Shows any texture (e.g. the shadow map) in a corner of the screen.
    void drawDebugTexture(GLuint texture, int x, int y, int w, int h);
    void destroy();

    int width = 0, height = 0;

private:
    GLuint sceneFbo = 0, sceneColor = 0, sceneDepth = 0;
    GLuint pingFbo[2] = {0, 0}, pingTex[2] = {0, 0};
    GLuint emptyVao = 0;   // the full-screen triangle needs a VAO bound
    Shader brightShader, blurShader, compositeShader, debugShader;

    void createTargets();
    void deleteTargets();
    void fullScreen() const;
};
