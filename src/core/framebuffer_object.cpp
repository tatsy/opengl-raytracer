#define GLRT_API_EXPORT
#include "framebuffer_object.h"

// ---------------------------------------------------------------------------------------------------------------------
// PUBLIC methods
// ---------------------------------------------------------------------------------------------------------------------

FramebufferObject::FramebufferObject(int width, int height)
    : width{ width }
    , height{ height } {
    initialize();
}

FramebufferObject::FramebufferObject(int width, int height, GLenum internalFormat, GLenum format, GLenum type)
    : width{ width }
    , height{ height }
    , internalFormat{ internalFormat }
    , format{ format }
    , type{ type } {
    initialize();
}

FramebufferObject::~FramebufferObject() {
    for (GLuint textureId : textures) {
        if (textureId != 0) {
            glDeleteTextures(1, &textureId);
        }
    }
    textures.clear();

    if (zBufferId != 0) {
        glDeleteRenderbuffers(1, &zBufferId);
    }

    if (fboId != 0) {
        glDeleteFramebuffers(1, &fboId);
    }
}

void FramebufferObject::addColorAttachment(int width, int height, GLenum internalFormat, GLenum format, GLenum type) {
    if (fboId == 0) {
        FatalError("Framebuffer object must be initialized before adding color attachments!");
    }

    if (width == 0) width = this->width;
    if (height == 0) height = this->height;
    if (internalFormat == GL_NONE) internalFormat = this->internalFormat;
    if (format == GL_NONE) format = this->format;
    if (type == GL_NONE) type = this->type;

    // Generate texture
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, nullptr);
    textures.push_back(textureId);

    // Generate framebuffer
    const int attachmentId = (int)attachments.size();
    glBindFramebuffer(GL_FRAMEBUFFER, fboId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachmentId, GL_TEXTURE_2D, textureId, 0);
    attachments.push_back(GL_COLOR_ATTACHMENT0 + attachmentId);

    // Unbind buffers
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FramebufferObject::setRenderTargets(int start, int size) {
    glDrawBuffers(size, (const GLenum *)&attachments[start]);
}

// ---------------------------------------------------------------------------------------------------------------------
// PRIVATE methods
// ---------------------------------------------------------------------------------------------------------------------

void FramebufferObject::initialize() {
    // Generate texture
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, nullptr);
    textures.push_back(textureId);

    // Generate zBuffer
    glGenRenderbuffers(1, &zBufferId);
    glBindRenderbuffer(GL_RENDERBUFFER, zBufferId);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);

    // Generate FBO
    glGenFramebuffers(1, &fboId);
    glBindFramebuffer(GL_FRAMEBUFFER, fboId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, zBufferId);
    attachments.push_back(GL_COLOR_ATTACHMENT0);

    // Unbind buffers
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}