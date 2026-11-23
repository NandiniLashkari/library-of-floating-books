// Shader.h
// Loads a vertex + fragment shader pair from the shaders/ folder, compiles and
// links them into one GPU program, and offers small helpers to set uniforms.
#pragma once

#include "GLCommon.h"
#include <string>

class Shader {
public:
    GLuint id = 0;

    // Reads both files, compiles them and links the program.
    // Returns false (and prints the GLSL error log) if anything fails.
    bool load(const std::string& vertexPath, const std::string& fragmentPath);

    void use() const { glUseProgram(id); }

    // Uniform setters: look the name up in the program and upload the value.
    void setInt(const char* name, int value) const;
    void setFloat(const char* name, float value) const;
    void setVec2(const char* name, const glm::vec2& v) const;
    void setVec3(const char* name, const glm::vec3& v) const;
    void setMat4(const char* name, const glm::mat4& m) const;
    // Uploads an array, e.g. setVec3Array("pointPos[0]", 12, data)
    void setVec3Array(const char* name, int count, const glm::vec3* data) const;

    void destroy();
};
