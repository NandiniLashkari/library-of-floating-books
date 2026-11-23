// GLCommon.h
// One place that pulls in OpenGL, GLFW and GLM, so every other file just
// includes this header.
#pragma once

#define GL_SILENCE_DEPRECATION   // macOS marks OpenGL as deprecated; hide those warnings
#include <OpenGL/gl3.h>          // OpenGL 4.1 Core Profile functions (macOS)
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
