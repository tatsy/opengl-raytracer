#ifdef _MSC_VER
#pragma once
#endif

#ifndef FRAMEBUFFER_OBJECT_H
#define FRAMEBUFFER_OBJECT_H

#include <cstdio>
#include <vector>

#include "api.h"
#include "common.h"
#include "uncopyable.h"

class GLRT_API FramebufferObject : private Uncopyable {
public:
    // PUBLIC methods
    FramebufferObject(int width, int height);
    FramebufferObject(int width, int height, GLenum internalFormat, GLenum format, GLenum type);
    virtual ~FramebufferObject();

    void addColorAttachment(int width = 0, int height = 0, GLenum internalFormat = GL_NONE, GLenum format = GL_NONE, GLenum type = GL_NONE);
    void setRenderTargets(int start, int size);

    inline void bind() { glBindFramebuffer(GL_FRAMEBUFFER, fboId); }

    inline void unbind() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawBuffer(GL_FRONT);
    }

    inline GLuint getId() const { return fboId; }

    inline GLuint textureId(int i = 0) const { return textures[i]; }

    inline size_t numTextures() const { return textures.size(); }

private:
    // PRIVATE methods
    void initialize();

    // PRIVATE parameters
    int width, height;
    GLenum internalFormat = GL_RGBA8;
    GLenum format = GL_RGBA;
    GLenum type = GL_UNSIGNED_BYTE;
    std::vector<GLuint> textures;
    std::vector<GLuint> attachments;
    GLuint fboId = 0;
    GLuint zBufferId = 0;
};

#endif  // FRAMEBUFFER_OBJECT_H
