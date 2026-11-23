// PostProcess.cpp
#include "PostProcess.h"
#include <iostream>

// Makes a colour texture that can be rendered into.
static GLuint makeColorTexture(int w, int h) {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    // RGBA16F = 16-bit floats per channel: values above 1.0 are kept (HDR).
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tex;
}

bool PostProcess::create(int w, int h) {
    bool ok = brightShader.load("shaders/post.vert", "shaders/bright.frag") &&
              blurShader.load("shaders/post.vert", "shaders/blur.frag") &&
              compositeShader.load("shaders/post.vert", "shaders/composite.frag") &&
              debugShader.load("shaders/post.vert", "shaders/debugquad.frag");
    glGenVertexArrays(1, &emptyVao);
    width = w;
    height = h;
    createTargets();
    return ok;
}

void PostProcess::createTargets() {
    // --- full-size HDR scene target: colour texture + depth buffer ---
    sceneColor = makeColorTexture(width, height);
    glGenRenderbuffers(1, &sceneDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, sceneDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);

    glGenFramebuffers(1, &sceneFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           sceneColor, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                              sceneDepth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "HDR framebuffer is incomplete\n";

    // --- two half-size targets for the blur ping-pong ---
    for (int i = 0; i < 2; ++i) {
        pingTex[i] = makeColorTexture(width / 2, height / 2);
        glGenFramebuffers(1, &pingFbo[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, pingFbo[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               pingTex[i], 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcess::deleteTargets() {
    glDeleteFramebuffers(1, &sceneFbo);
    glDeleteTextures(1, &sceneColor);
    glDeleteRenderbuffers(1, &sceneDepth);
    glDeleteFramebuffers(2, pingFbo);
    glDeleteTextures(2, pingTex);
}

void PostProcess::resize(int w, int h) {
    if (w == width && h == height) return;
    width = w;
    height = h;
    deleteTargets();
    createTargets();
}

void PostProcess::beginScene() {
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFbo);
    glViewport(0, 0, width, height);
    glClearColor(0.012f, 0.009f, 0.008f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
}

void PostProcess::fullScreen() const {
    glBindVertexArray(emptyVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void PostProcess::finish(bool bloomOn, float exposure, bool passthrough) {
    glDisable(GL_DEPTH_TEST);

    if (bloomOn) {
        // 1. bright pass into ping[0] (half resolution)
        glViewport(0, 0, width / 2, height / 2);
        glBindFramebuffer(GL_FRAMEBUFFER, pingFbo[0]);
        brightShader.use();
        brightShader.setInt("hdrScene", 0);
        brightShader.setFloat("threshold", 1.0f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneColor);
        fullScreen();

        // 2. blur: ping -> pong horizontally, pong -> ping vertically, x5
        blurShader.use();
        blurShader.setInt("image", 0);
        for (int i = 0; i < 10; ++i) {
            int horizontal = (i % 2 == 0) ? 1 : 0;
            glBindFramebuffer(GL_FRAMEBUFFER, pingFbo[horizontal]);
            blurShader.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, pingTex[1 - horizontal]);
            fullScreen();
        }
    }

    // 3. composite to the screen (framebuffer 0)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    compositeShader.use();
    compositeShader.setInt("hdrScene", 0);
    compositeShader.setInt("bloom", 1);
    compositeShader.setInt("bloomOn", bloomOn ? 1 : 0);
    compositeShader.setFloat("bloomStrength", 0.9f);
    compositeShader.setFloat("exposure", exposure);
    compositeShader.setInt("passthrough", passthrough ? 1 : 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneColor);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pingTex[0]);
    fullScreen();
    glActiveTexture(GL_TEXTURE0);
}

void PostProcess::drawDebugTexture(GLuint texture, int x, int y, int w, int h) {
    glViewport(x, y, w, h);
    debugShader.use();
    debugShader.setInt("depthTexture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    fullScreen();
    glViewport(0, 0, width, height);
}

void PostProcess::destroy() {
    deleteTargets();
    glDeleteVertexArrays(1, &emptyVao);
    brightShader.destroy();
    blurShader.destroy();
    compositeShader.destroy();
    debugShader.destroy();
}
