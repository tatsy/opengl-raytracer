#ifdef MSC_VER
#pragma once
#endif

#ifndef TEXTURE_H
#define TEXTURE_H

#include <string>

#include "api.h"
#include "common.h"
#include "uncopyable.h"

class GLRT_API Texture : private Uncopyable {
public:
    // PUBLIC methods
    Texture();
    explicit Texture(const std::string &filename, bool generateMipMap = false);
    Texture(int width, int height, GLenum internalFormat = GL_RGBA8,
            GLenum format = GL_RGBA, GLenum type = GL_UNSIGNED_BYTE);
    Texture(int width, int height);
    virtual ~Texture();

    void load(const std::string &filename, bool generateMipMap = false);

    void clear(const glm::vec4 &color);

    void bind(GLuint i = 0) const {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textureId);
    }

    void unbind(GLuint i = 0) const {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    GLuint getId() const { return textureId; }
    int width() const { return width_; }
    int height() const { return height_; }

private:
    // PRIVATE parameters
    int width_, height_, channels_;
    GLenum internalFormat_, format_, type_;
    GLuint textureId = 0;
};

#endif  // TEXTURE_H
