// Shader.cpp
#include "Shader.h"

#include <fstream>
#include <sstream>
#include <iostream>

// Read a whole text file into a string.
static std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Could not open shader file: " << path << '\n';
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Compile one shader stage and print the error log if it fails.
static GLuint compileStage(GLenum type, const std::string& source,
                           const std::string& path) {
    GLuint shader = glCreateShader(type);
    const char* text = source.c_str();
    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "Shader compile error in " << path << ":\n" << log << '\n';
    }
    return shader;
}

bool Shader::load(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vsText = readFile(vertexPath);
    std::string fsText = readFile(fragmentPath);
    if (vsText.empty() || fsText.empty()) return false;

    GLuint vs = compileStage(GL_VERTEX_SHADER, vsText, vertexPath);
    GLuint fs = compileStage(GL_FRAGMENT_SHADER, fsText, fragmentPath);

    id = glCreateProgram();
    glAttachShader(id, vs);
    glAttachShader(id, fs);
    glLinkProgram(id);

    GLint ok = GL_FALSE;
    glGetProgramiv(id, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetProgramInfoLog(id, sizeof(log), nullptr, log);
        std::cerr << "Shader link error (" << vertexPath << " + "
                  << fragmentPath << "):\n" << log << '\n';
    }

    // The program keeps its own copy, so the stage objects can go.
    glDeleteShader(vs);
    glDeleteShader(fs);
    return ok == GL_TRUE;
}

void Shader::setInt(const char* name, int value) const {
    glUniform1i(glGetUniformLocation(id, name), value);
}
void Shader::setFloat(const char* name, float value) const {
    glUniform1f(glGetUniformLocation(id, name), value);
}
void Shader::setVec2(const char* name, const glm::vec2& v) const {
    glUniform2fv(glGetUniformLocation(id, name), 1, glm::value_ptr(v));
}
void Shader::setVec3(const char* name, const glm::vec3& v) const {
    glUniform3fv(glGetUniformLocation(id, name), 1, glm::value_ptr(v));
}
void Shader::setMat4(const char* name, const glm::mat4& m) const {
    glUniformMatrix4fv(glGetUniformLocation(id, name), 1, GL_FALSE,
                       glm::value_ptr(m));
}
void Shader::setVec3Array(const char* name, int count, const glm::vec3* data) const {
    glUniform3fv(glGetUniformLocation(id, name), count,
                 reinterpret_cast<const float*>(data));
}

void Shader::destroy() {
    if (id) glDeleteProgram(id);
    id = 0;
}
