#pragma once

#include "api.h"
#include "common.h"

class GLRT_API TextureBuffer {
public:
    TextureBuffer(size_t size, GLenum internalFormat, GLenum usage);
    virtual ~TextureBuffer();

    void bind(int id = 0);
    void setData(void *data);

private:
    void initialize();

    GLuint texId = 0u, bufId = 0u;
    size_t size;
    GLenum internalFormat, usage;
};
