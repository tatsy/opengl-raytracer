#ifdef _MSC_VER
#pragma once
#endif

#ifndef SHADER_STAGE_H
#define SHADER_STAGE_H

#include <string>

#include "api.h"
#include "common.h"

//! ShaderType enumeration
enum class ShaderType : int {
    Vertex = GL_VERTEX_SHADER,
    Geometry = GL_GEOMETRY_SHADER,
    TessControl = GL_TESS_CONTROL_SHADER,
    TessEvaluation = GL_TESS_EVALUATION_SHADER,
    Fragment = GL_FRAGMENT_SHADER,
    Compute = GL_COMPUTE_SHADER
};

//! ShaderStage manages the process in each shader stage.
class GLRT_API ShaderStage {
public:
    // PUBLIC methods
    ShaderStage(ShaderStage &&shader) noexcept;
    ShaderStage &operator=(ShaderStage &&shader) noexcept;
    virtual ~ShaderStage();

    static ShaderStage fromFile(const std::string &dirname, const std::string &filename, ShaderType type);
    static ShaderStage fromFile(const std::string &filename, ShaderType type);
    static ShaderStage fromSource(const std::string &source, ShaderType type);
    void compile(const std::string &source, ShaderType type);

    inline GLuint operator()() const { return shaderId; }

private:
    // PRIVATE methods
    ShaderStage();

    // PRIVATE parameters
    uint32_t shaderId = 0;
};

#endif  // SHADER_STAGE_H
