// main.cpp
// Library of Floating Books - CS7GV6 Computer Graphics, Deliverable 3
//
// Frame overview:
//   1. input + animation clocks + lights
//   2. SHADOW PASS  : depth from the moon into ShadowMap   (shadow.vert/frag)
//   3. SCENE PASS   : lit, textured scene into an HDR buffer (scene.vert/frag)
//   4. POST PASS    : bloom + tone mapping + gamma to the screen (PostProcess)
//   5. OVERLAY      : on-screen section titles and feature switches (TextOverlay)
//
// Keys: WASD/QE/Shift move, mouse or arrows look, ESC free mouse (twice = quit),
//       M capture mouse, TAB section title, H hide labels, P/C hierarchy,
//       0-9 and F feature switches (see keyCallback below).

#include "GLCommon.h"
#include "Shader.h"
#include "Mesh.h"
#include "Texture.h"
#include "Camera.h"
#include "Settings.h"
#include "Lights.h"
#include "Renderer.h"
#include "Library.h"
#include "FloatingBooks.h"
#include "ShadowMap.h"
#include "PostProcess.h"
#include "TextOverlay.h"

#include <iostream>
#include <string>
#include <cstdio>

// ---------------------------------------------------------------- globals
// (GLFW callbacks are plain C functions, so they reach these directly)
static Camera camera;
static Settings settings;
static bool mouseCaptured = true;
static bool firstMouse = true;
static double lastMouseX = 0.0, lastMouseY = 0.0;

static void showCaption(const std::string& text) {
    settings.caption = text;
    settings.captionTimer = 5.0f;
}
static const char* onOff(bool b) { return b ? "ON" : "OFF"; }

// ---------------------------------------------------------------- input
static void cursorCallback(GLFWwindow*, double x, double y) {
    if (!mouseCaptured) { firstMouse = true; return; }
    if (firstMouse) { lastMouseX = x; lastMouseY = y; firstMouse = false; }
    camera.turn(static_cast<float>(x - lastMouseX), static_cast<float>(y - lastMouseY));
    lastMouseX = x;
    lastMouseY = y;
}

static void keyCallback(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    Settings& s = settings;

    switch (key) {
    case GLFW_KEY_ESCAPE:
        if (mouseCaptured) {
            mouseCaptured = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        break;
    case GLFW_KEY_M:
        mouseCaptured = true;
        firstMouse = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        break;

    // ---- Section 1: Lighting & Shading ----
    case GLFW_KEY_1:
        s.viewMode = (s.viewMode + 1) % 5;
        if (s.viewMode == 0) showCaption("Full Blinn-Phong: ambient + diffuse + specular, 14 lights");
        if (s.viewMode == 1) showCaption("Ambient only: constant base light, no direction");
        if (s.viewMode == 2) showCaption("Diffuse only: Lambert  max(N.L, 0) per light");
        if (s.viewMode == 3) showCaption("Specular only: Blinn-Phong  max(N.H, 0)^shininess");
        if (s.viewMode == 4) showCaption("Normals as colours (includes normal maps)");
        break;
    case GLFW_KEY_2:
        s.moonOn = !s.moonOn;
        showCaption(std::string("Moonlight (directional light) ") + onOff(s.moonOn));
        break;
    case GLFW_KEY_3:
        s.candlesOn = !s.candlesOn;
        showCaption(std::string("Candles: 12 point lights + magic light, attenuated ") +
                    onOff(s.candlesOn));
        break;

    case GLFW_KEY_0:
        s.shadingModel = (s.shadingModel + 1) % 3;
        if (s.shadingModel == 0) showCaption("Phong shading: lighting computed per pixel");
        if (s.shadingModel == 1) showCaption("Gouraud shading: lighting per vertex, interpolated");
        if (s.shadingModel == 2) showCaption("Flat shading: one normal per triangle");
        break;

    // ---- Section 2: Texture Mapping ----
    case GLFW_KEY_4:
        s.texturesOn = !s.texturesOn;
        showCaption(std::string("Image textures ") + onOff(s.texturesOn) +
                    (s.texturesOn ? "" : "  (plain material colours)"));
        break;

    case GLFW_KEY_F:
        s.filterMode = (s.filterMode + 1) % 4;
        showCaption(std::string("Texture filtering: ") + filterModeName(s.filterMode));
        break;

    // ---- Section 3: Advanced Features ----
    case GLFW_KEY_5:
        s.normalMapOn = !s.normalMapOn;
        showCaption(std::string("Normal mapping ") + onOff(s.normalMapOn));
        break;
    case GLFW_KEY_6:
        s.shadowsOn = !s.shadowsOn;
        showCaption(std::string("Shadow mapping (moonlight, 5x5 PCF) ") + onOff(s.shadowsOn));
        break;
    case GLFW_KEY_7:
        s.bloomOn = !s.bloomOn;
        showCaption(std::string("HDR bloom ") + onOff(s.bloomOn));
        break;
    case GLFW_KEY_8:
        s.reflectionsOn = !s.reflectionsOn;
        showCaption(std::string("Cubemap reflections on the crystal ") + onOff(s.reflectionsOn));
        break;
    case GLFW_KEY_9:
        s.showShadowMap = !s.showShadowMap;
        showCaption(std::string("Shadow map (depth from the moon) ") + onOff(s.showShadowMap));
        break;

    // ---- Hierarchy + labels ----
    case GLFW_KEY_P:
        s.parentOn = !s.parentOn;
        showCaption(std::string("Parent animation ") + onOff(s.parentOn));
        break;
    case GLFW_KEY_C:
        s.childOn = !s.childOn;
        showCaption(std::string("Children animation ") + onOff(s.childOn));
        break;
    case GLFW_KEY_TAB:
        s.section = (s.section + 1) % 4;
        break;
    case GLFW_KEY_H:
        s.showHud = !s.showHud;
        break;
    default:
        break;
    }
}

// ---------------------------------------------------------------- overlay
static void drawOverlay(TextOverlay& hud, int w, int h) {
    const Settings& s = settings;
    if (!s.showHud) return;

    float k = h / 720.0f;              // scale labels with the window size
    float size = 19.0f * k;
    const glm::vec4 bright(1.0f, 0.93f, 0.75f, 1.0f);
    const glm::vec4 dim(0.70f, 0.66f, 0.60f, 1.0f);
    const glm::vec4 panelColor(0.02f, 0.02f, 0.04f, 0.62f);

    hud.begin(w, h);

    // Big section banner at the top.
    const char* sections[4] = {"", "1. LIGHTING & SHADING", "2. TEXTURE MAPPING",
                               "3. ADVANCED FEATURES"};
    if (s.section > 0) {
        float bs = 40.0f * k;
        float bw = hud.width(sections[s.section], bs);
        hud.panel((w - bw) / 2 - 20 * k, 18 * k, bw + 40 * k, bs + 20 * k, panelColor);
        hud.text(sections[s.section], (w - bw) / 2, 28 * k, bs, bright);
    }

    // Feature list (bottom-left). Lines of the current section are bright.
    struct Line { std::string text; int section; };
    std::string view = std::string("[1] View: ") + viewModeName(s.viewMode);
    Line lines[] = {
        {view, 1},
        {std::string("[2] Moonlight: ") + onOff(s.moonOn), 1},
        {std::string("[3] Candle lights: ") + onOff(s.candlesOn), 1},
        {std::string("[0] Shading: ") + shadingModelName(s.shadingModel), 1},
        {std::string("[4] Textures: ") + onOff(s.texturesOn), 2},
        {std::string("[F] Filtering: ") + filterModeName(s.filterMode), 2},
        {std::string("[5] Normal mapping: ") + onOff(s.normalMapOn), 3},
        {std::string("[6] Shadows: ") + onOff(s.shadowsOn), 3},
        {std::string("[7] HDR bloom: ") + onOff(s.bloomOn), 3},
        {std::string("[8] Reflections: ") + onOff(s.reflectionsOn), 3},
        {std::string("[9] Shadow map view: ") + onOff(s.showShadowMap), 3},
        {std::string("[P] Parent: ") + onOff(s.parentOn) +
             "  [C] Children: " + onOff(s.childOn), 0},
    };
    const int n = sizeof(lines) / sizeof(lines[0]);
    float lineH = size * 1.25f;
    float px = 16 * k, py = h - 16 * k - n * lineH - 12 * k;
    hud.panel(px, py, 420 * k, n * lineH + 12 * k, panelColor);
    for (int i = 0; i < n; ++i) {
        bool hot = (s.section == 0) || (lines[i].section == s.section);
        hud.text(lines[i].text, px + 10 * k, py + 6 * k + i * lineH, size, hot ? bright : dim);
    }

    // Caption explaining the last key pressed (bottom-right).
    if (s.captionTimer > 0.0f && !s.caption.empty()) {
        float cs = 21.0f * k;
        float cw = hud.width(s.caption, cs);
        float alpha = glm::min(1.0f, s.captionTimer);   // fade out in the last second
        hud.panel(w - cw - 36 * k, h - cs - 40 * k, cw + 20 * k, cs + 16 * k,
                  glm::vec4(0.02f, 0.02f, 0.04f, 0.7f * alpha));
        hud.text(s.caption, w - cw - 26 * k, h - cs - 32 * k, cs,
                 glm::vec4(1.0f, 0.85f, 0.5f, alpha));
    }

    hud.end();
}

// ---------------------------------------------------------------- main
int main() {
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed.\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Library of Floating Books",
                                          nullptr, nullptr);
    if (!window) {
        std::cerr << "Window creation failed.\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetCursorPosCallback(window, cursorCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // ---- shaders ----
    Shader sceneShader, shadowShader;
    if (!sceneShader.load("shaders/scene.vert", "shaders/scene.frag") ||
        !shadowShader.load("shaders/shadow.vert", "shaders/shadow.frag")) {
        std::cerr << "Run the program from the project folder (it needs shaders/).\n";
        return -1;
    }

    // ---- scene content ----
    Mesh cube = createCube();
    Library library;
    FloatingBooks floatingBooks;
    if (!library.load() || !floatingBooks.load()) {
        std::cerr << "Some models or textures failed to load (see above).\n";
    }

    Renderer renderer;
    renderer.cube = &cube;
    renderer.white = Texture::solid(255, 255, 255, Texture::Kind::Color);
    renderer.flatNormal = Texture::solid(128, 128, 255, Texture::Kind::Data);
    renderer.defaultOrm = Texture::solid(255, 150, 0, Texture::Kind::Data);

    Lights lights;
    ShadowMap shadowMap;
    shadowMap.create();

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);   // pixels (Retina = 2x)
    PostProcess post;
    TextOverlay hud;
    if (!post.create(fbWidth, fbHeight) || !hud.create("models/textures/font.png")) {
        std::cerr << "Post-processing or text overlay failed to load.\n";
    }

    const float fieldOfView = glm::radians(50.0f);
    double lastFrame = glfwGetTime();
    double fpsTimer = 0.0;
    int appliedFilter = 3;   // textures are created trilinear + anisotropic
    int fpsFrames = 0;

    while (!glfwWindowShouldClose(window)) {
        // ---------------- 1. time, input, animation ----------------
        double now = glfwGetTime();
        float deltaTime = static_cast<float>(now - lastFrame);
        lastFrame = now;
        float time = static_cast<float>(now);

        camera.move(window, deltaTime);
        floatingBooks.update(deltaTime, settings);
        lights.update(time, floatingBooks.parentMatrix, settings);
        if (settings.captionTimer > 0.0f) settings.captionTimer -= deltaTime;
        if (settings.filterMode != appliedFilter) {      // texture filtering changed
            library.setFiltering(settings.filterMode);
            appliedFilter = settings.filterMode;
        }

        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        if (fbWidth <= 0 || fbHeight <= 0) { glfwPollEvents(); continue; }
        post.resize(fbWidth, fbHeight);

        // ---------------- 2. shadow pass ----------------
        shadowMap.updateLight(lights.moonDirection);
        bool drawShadows = settings.shadowsOn && settings.moonOn;
        if (drawShadows || settings.showShadowMap) {
            shadowMap.begin();
            shadowShader.use();
            shadowShader.setMat4("lightSpace", shadowMap.lightSpace);
            renderer.shader = &shadowShader;
            renderer.depthPass = true;
            library.draw(renderer, lights, time);
            floatingBooks.draw(renderer);
            shadowMap.end(fbWidth, fbHeight);
        }

        // ---------------- 3. scene pass (HDR) ----------------
        post.beginScene();
        sceneShader.use();
        glm::mat4 projection = glm::perspective(
            fieldOfView, static_cast<float>(fbWidth) / fbHeight, 0.05f, 100.0f);
        sceneShader.setMat4("view", camera.viewMatrix());
        sceneShader.setMat4("projection", projection);
        sceneShader.setMat4("lightSpace", shadowMap.lightSpace);
        sceneShader.setVec3("cameraPos", camera.position);
        sceneShader.setFloat("time", time);
        sceneShader.setInt("viewMode", settings.viewMode);
        sceneShader.setInt("shadingModel", settings.shadingModel);
        sceneShader.setInt("texturesOn", settings.texturesOn ? 1 : 0);
        sceneShader.setInt("normalMappingOn", settings.normalMapOn ? 1 : 0);
        sceneShader.setInt("shadowsOn", drawShadows ? 1 : 0);
        sceneShader.setInt("reflectionsOn", settings.reflectionsOn ? 1 : 0);
        lights.upload(sceneShader);

        // Texture units: 0-2 per object (set by Renderer), 3-5 shared.
        sceneShader.setInt("albedoMap", 0);
        sceneShader.setInt("normalMap", 1);
        sceneShader.setInt("ormMap", 2);
        sceneShader.setInt("skybox", 3);
        sceneShader.setInt("shadowMap", 4);
        sceneShader.setInt("rugMap", 5);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_CUBE_MAP, library.sky);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, shadowMap.depthTexture);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, library.rug);

        renderer.shader = &sceneShader;
        renderer.depthPass = false;
        library.draw(renderer, lights, time);
        floatingBooks.draw(renderer);

        // ---------------- 4. post-processing to the screen ----------------
        post.finish(settings.bloomOn, 1.0f, settings.viewMode == 4);
        if (settings.showShadowMap) {
            int s = fbHeight / 3;
            post.drawDebugTexture(shadowMap.depthTexture, fbWidth - s - 20,
                                  fbHeight - s - 20, s, s);
        }

        // ---------------- 5. labels ----------------
        drawOverlay(hud, fbWidth, fbHeight);

        // FPS in the title bar
        ++fpsFrames;
        fpsTimer += deltaTime;
        if (fpsTimer > 1.0) {
            char title[128];
            std::snprintf(title, sizeof(title), "Library of Floating Books  |  %.0f FPS",
                          fpsFrames / fpsTimer);
            glfwSetWindowTitle(window, title);
            fpsFrames = 0;
            fpsTimer = 0.0;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    hud.destroy();
    post.destroy();
    shadowMap.destroy();
    cube.destroy();
    sceneShader.destroy();
    shadowShader.destroy();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
