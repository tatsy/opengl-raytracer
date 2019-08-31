#ifdef _MSC_VER
#pragma once
#endif

#ifndef SHADER_PROGRAM_H
#define SHADER_PROGRAM_H

#include <glm/glm.hpp>
#include <string>

#include "api.h"
#include "common.h"
#include "shader_stage.h"

/**
 *ShaderProgram wraps shader stages into a program.
 */
class GLRT_API ShaderProgram {
public:
    // PUBLIC methods
    ShaderProgram();
    virtual ~ShaderProgram();

    void create();
    void attachShader(const ShaderStage &shader);
    void link();
    void start() const;
    void end() const;

    GLint getUniformLocation(const std::string &name);
    void setUniform1i(const std::string &name, int v);
    void setUniform2i(const std::string &name, const glm::ivec2 &v);
    void setUniform3i(const std::string &name, const glm::ivec3 &v);
    void setUniform4i(const std::string &name, const glm::ivec4 &v);

    void setUniform1f(const std::string &name, float v);
    void setUniform1fv(const std::string &name, const float *v, size_t size);
    void setUniform2f(const std::string &name, const glm::vec2 &v);
    void setUniform2fv(const std::string &name, const glm::vec2 *v, size_t size);
    void setUniform3f(const std::string &name, const glm::vec3 &v);
    void setUniform3fv(const std::string &name, const glm::vec3 *v, size_t size);
    void setUniform4f(const std::string &name, const glm::vec4 &v);
    void setUniform4fv(const std::string &name, const glm::vec4 *v, size_t size);
    void setMatrix4x4(const std::string &name, const glm::mat4 &m);

private:
    // PRIVATE parameters
    GLuint programId = 0;
};

#endif  // SHADER_PROGRAM_H
