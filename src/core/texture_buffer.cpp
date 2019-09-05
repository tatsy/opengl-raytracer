#define GLRT_API_EXPORT
#include "texture_buffer.h"

TextureBuffer::TextureBuffer(size_t size, GLenum internalFormat, GLenum usage)
    : size(size)
    , internalFormat(internalFormat)
    , usage(usage) {
    initialize();
}

TextureBuffer::~TextureBuffer() {
    if (texId != 0u) {
        glDeleteTextures(1, &texId);
        texId = 0u;
    }

    if (bufId != 0u) {
        glDeleteBuffers(1, &bufId);
        bufId = 0u;
    }
}

void TextureBuffer::bind(int id) {
    glActiveTexture(GL_TEXTURE0 + id);
    glBindTexture(GL_TEXTURE_BUFFER, texId);
}

void TextureBuffer::initialize() {
    glGenBuffers(1, &bufId);
    glBindBuffer(GL_TEXTURE_BUFFER, bufId);
    glBufferData(GL_TEXTURE_BUFFER, size, nullptr, usage);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);

    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_BUFFER, texId);
    glTexBuffer(GL_TEXTURE_BUFFER, internalFormat, bufId);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
}

void TextureBuffer::setData(void* data) {
    glBindBuffer(GL_TEXTURE_BUFFER, bufId);
    glBufferSubData(GL_TEXTURE_BUFFER, 0, size, data);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
}
