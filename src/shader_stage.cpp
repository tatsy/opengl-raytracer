#define GLRT_API_EXPORT
#include "shader_stage.h"

#include <fstream>
#include <iostream>
#include <regex>
#include <unordered_map>

#include "common.h"
#include "argparse.h"

// ---------------------------------------------------------------------------------------------------------------------
// Utility functions
// ---------------------------------------------------------------------------------------------------------------------

struct GLSLError {
    GLSLError() {}
    GLSLError(int lineno, const std::string &msg)
        : lineno(lineno)
        , msg(msg) {
    }
    int lineno;
    std::string msg;
};

void handleGLSLErrors(const std::string &errMsg, const std::string &source) {
    // Regex for parsing error messages
    std::regex pattern("[\\(]{0,1}([0-9]+)[:\\)]{0,1}[ ]{0,1}: ([0-9a-zA-Z -/:-@\\[-~]+)");
    std::smatch match;
    int pos;

    // Parse error messages
    std::unordered_map<int, std::string> errors;
    pos = 0;
    while (true) {
        const int cur = errMsg.find_first_of('\n', pos);
        const std::string line = errMsg.substr(pos, cur - pos);
        if (line.length() == 0) {
            continue;
        }

        std::cout << line << std::endl;
        if (std::regex_search(line, match, pattern)) {
            int lineno = atoi(match.str(1).c_str());
            errors[lineno] = match.str(2);
        }

        if (cur == std::string::npos) {
            break;
        }
        pos = cur + 1;
    }

    pos = 0;
    int lineno = 0;
    while (true) {
        lineno += 1;

        const int cur = source.find_first_of('\n', pos);
        if (errors.find(lineno) != errors.end()) {
            printf("*%3d", lineno);
        } else {
            printf(" %3d", lineno);
        }

        const std::string line = source.substr(pos, cur - pos);
        printf(" | %s\n", line.c_str());
        if (cur == std::string::npos) {
            break;
        }
        pos = cur + 1;
    }
}


// ---------------------------------------------------------------------------------------------------------------------
// PUBLIC methods
// ---------------------------------------------------------------------------------------------------------------------

ShaderStage::ShaderStage(ShaderStage &&shader) noexcept {
    shaderId = shader.shaderId;
    shader.shaderId = 0;
}

ShaderStage &ShaderStage::operator=(ShaderStage &&shader) noexcept {
    shaderId = shader.shaderId;
    shader.shaderId = 0;
    return *this;
}

ShaderStage::~ShaderStage() {
    if (shaderId != 0) {
        glDeleteShader(shaderId);
        shaderId = 0;
    }
}

ShaderStage ShaderStage::fromFile(const std::string &filename, ShaderType type) {
    // Full source file path
    auto &parser = ArgumentParser::getInstance();
    const std::string appPath = parser.getExecutablePath();
    const std::string shaderDir = fs::canonical(fs::path(appPath.c_str()).parent_path() / fs::path("../shaders")).string();
    return ShaderStage::fromFile(shaderDir, filename, type);
}

ShaderStage ShaderStage::fromFile(const std::string &dirname, const std::string &filename, ShaderType type) {
    // Load source file
    const std::string shaderFile = fs::absolute(fs::path(dirname) / fs::path(filename.c_str())).string();
    std::ifstream reader(shaderFile.c_str());
    if (!reader.is_open()) {
        FatalError("Failed to open shader source: %s", shaderFile.c_str());
    }

    std::string code;
    reader.seekg(0, std::ios::end);
    code.reserve(reader.tellg());
    reader.seekg(0, std::ios::beg);    
    code.assign(std::istreambuf_iterator<char>(reader), std::istreambuf_iterator<char>());
    reader.close();

    // Compile
    ShaderStage shader;
    shader.compile(code, type);

    return std::move(shader);
}

ShaderStage ShaderStage::fromSource(const std::string &source, ShaderType type) {
    ShaderStage shader;
    shader.compile(source, type);
    return std::move(shader);
}

void ShaderStage::compile(const std::string &source, ShaderType type) {
    // Create
    shaderId = glCreateShader((GLuint)type);

    // Compile
    const char *codePtr = source.c_str();
    glShaderSource(shaderId, 1, &codePtr, nullptr);
    glCompileShader(shaderId);

    // Error handling
    GLint compileStatus;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_FALSE) {
        // Show error message and source code, then terminate.
        fprintf(stderr, "[ERROR] Failed to compile a shader!\n");

        GLint logLength;
        glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            GLsizei length;
            std::string errMsg;
            errMsg.resize((size_t)logLength);
            glGetShaderInfoLog(shaderId, logLength, &length, &errMsg[0]);
            handleGLSLErrors(errMsg, source);
        }
        std::abort();
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// PRIVATE methods
// ---------------------------------------------------------------------------------------------------------------------

ShaderStage::ShaderStage() {}