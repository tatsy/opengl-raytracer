#define GLRT_API_EXPORT
#include "shader_program.h"

#include <glm/gtc/type_ptr.hpp>

#include "common.h"

// ---------------------------------------------------------------------------------------------------------------------
// PUBLIC methods
// ---------------------------------------------------------------------------------------------------------------------

ShaderProgram::ShaderProgram() {}

ShaderProgram::~ShaderProgram() {
    if (programId != 0) {
        glDeleteProgram(programId);
    }
}

void ShaderProgram::create() { programId = glCreateProgram(); }

void ShaderProgram::attachShader(const ShaderStage &shader) { glAttachShader(programId, shader()); }

void ShaderProgram::link() {
    // Link
    glLinkProgram(programId);

    // Check status
    GLint linkStatus;
    glGetProgramiv(programId, GL_LINK_STATUS, &linkStatus);

    // Error handling
    if (linkStatus == GL_FALSE) {
        // Show error message and terminate if linkage failed
        fprintf(stderr, "[ERROR] Failed to link shaders!\n");

        GLint logLength;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            GLsizei length;
            std::string errMsg;
            errMsg.resize((size_t)logLength);
            glGetProgramInfoLog(programId, logLength, &length, &errMsg[0]);
            fprintf(stderr, "%s\n", errMsg.c_str());
        }
        std::abort();
    }

    // Inactivate
    glUseProgram(0);
}

void ShaderProgram::start() const { glUseProgram(programId); }

void ShaderProgram::end() const { glUseProgram(0); }

GLint ShaderProgram::getUniformLocation(const std::string &name) {
    GLint currentProgramId;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgramId);
    if (currentProgramId != programId) {
        Warn("Uniform variable %s is set before program is used!", name.c_str());
        return -1;
    }
    return glGetUniformLocation(programId, name.c_str());
}

void ShaderProgram::setUniform1i(const std::string &name, int v) {
    const GLint location = getUniformLocation(name);
    if (location >= 0) {
        glUniform1i(location, v);
    }
}

void ShaderProgram::setUniform2i(const std::string &name, const glm::ivec2 &v) {
    const GLint location = getUniformLocation(name);
    if (location >= 0) {
        glUniform2iv(location, 1, glm::value_ptr(v));
    }
}

void ShaderProgram::setUniform3i(const std::string &name, const glm::ivec3 &v) {
    const GLint location = getUniformLocation(name);
    if (location >= 0) {
        glUniform3iv(location, 1, glm::value_ptr(v));
    }
}

void ShaderProgram::setUniform4i(const std::string &name, const glm::ivec4 &v) {
    const GLint location = getUniformLocation(name);
    if (location >= 0) {
        glUniform4iv(location, 1, glm::value_ptr(v));
    }
}

void ShaderProgram::setUniform1f(const std::string &name, float v) {
    const GLint location = getUniformLocation(name);
    if (location >= 0) {
        glUniform1f(location, v);
    }
}

void ShaderProgram::setUniform1fv(const std::string &name, const float *v, size_t size) {
    const GLint location = getUniformLocation(name);
    if (location > 0) {
        glUniform1fv(location, (GLsizei)size, v);
    }
}

void ShaderProgram::setUniform2f(const std::string &name, const glm::vec2 &v) {
    const GLint location = getUniformLocation(name);
    if (location >= 0) {
        glUniform2fv(location, 1, glm::value_ptr(v));
    }
}

void ShaderProgram::setUniform2fv(const std::string &name, const glm::vec2 *v, size_t size) {
    const GLint location = getUniformLocation(name);
    if (location >= 0) {
        glUniform2fv(location, (GLsizei)size, (GLfloat *)v);
    }
}

void ShaderProgram::setUniform3f(const std::string &name, const glm::vec3 &v) {
    const GLint location = getUniformLocation(name);
    if (location >= 0) {
        glUniform3fv(location, 1, glm::value_ptr(v));
    }
}

void ShaderProgram::setUniform3fv(const std::string &name, const glm::vec3 *v, size_t size) {
    const GLint location = getUniformLocation(name);
    if (location >= 0) {
        glUniform3fv(location, (GLsizei)size, (GLfloat *)v);
    }
}

void ShaderProgram::setUniform4f(const std::string &name, const glm::vec4 &v) {
    const GLint location = getUniformLocation(name);
    if (location >= 0) {
        glUniform4fv(location, 1, glm::value_ptr(v));
    }
}

void ShaderProgram::setUniform4fv(const std::string &name, const glm::vec4 *v, size_t size) {
    const GLint location = getUniformLocation(name);
    if (location >= 0) {
        glUniform4fv(location, (GLsizei)size, (GLfloat *)v);
    }
}

void ShaderProgram::setMatrix4x4(const std::string &name, const glm::mat4 &m) {
    const GLint location = getUniformLocation(name);
    if (location >= 0) {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(m));
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// PRIVATE methods
// ---------------------------------------------------------------------------------------------------------------------
