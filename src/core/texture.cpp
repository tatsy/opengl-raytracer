#define GLRT_API_EXPORT
#include "texture.h"

#include <algorithm>
#include <experimental/filesystem>

#include <stb_image.h>

#include "common.h"

namespace fs = std::experimental::filesystem;

template <typename T>
void imageFlip(T *bytes, int width, int height, int channels) {
    for (int y = 0; y < height / 2; y++) {
        for (int x = 0; x < width; x++) {
            for (int c = 0; c < channels; c++) {
                T b = bytes[(y * width + x) * channels + c];
                bytes[(y * width + x) * channels + c] = bytes[((height - y - 1) * width + x) * channels + c];
                bytes[((height - y - 1) * width + x) * channels + c] = b;
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// PUBLIC methods
// ---------------------------------------------------------------------------------------------------------------------

Texture::Texture()
    : width_{ 0 }
    , height_{ 0 }
    , channels_{ 0 }
    , internalFormat_{ GL_RGBA8 }
    , format_{ GL_RGBA }
    , type_{ GL_UNSIGNED_BYTE } {
}

Texture::Texture(const std::string &filename, bool generateMipMap)
    : Texture{} {
    load(filename, generateMipMap);
}

Texture::Texture(int width, int height, GLenum internalFormat, GLenum format, GLenum type)
    : width_{ width }
    , height_{ height }
    , internalFormat_{ internalFormat }
    , format_{ format }
    , type_{ type } {

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width_, height_, 0, format, type, nullptr);
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glClearTexImage(GL_TEXTURE_2D, 0, format, type, clearColor);
    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::~Texture() {
    if (textureId != 0) {
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &textureId);
    }
}

void Texture::clear(const glm::vec4 &color) {
    glBindTexture(GL_TEXTURE_2D, textureId);
    float data[4] = { color.x, color.y, color.z, color.w };
    glClearTexImage(GL_TEXTURE_2D, 0, format_, type_, data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::load(const std::string &filename, bool generateMipMap) {
    // Check extension
    std::string extension = fs::path(filename).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    // Texture setup
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    if (extension == ".hdr") {
        // Float texture
        float *bytes = stbi_loadf(filename.c_str(), &width_, &height_, &channels_, STBI_rgb_alpha);
        if (!bytes) {
            FatalError("Failed to load image file: %s", filename.c_str());
        }

        // Inversion
        //imageFlip(bytes, width_, height_, 4);

        // Memory allocation
        if (!generateMipMap) {
            glTexStorage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width_, height_);
        } else {
            const int mipLevels = (int)std::ceil(std::log2(std::max(width_, height_)));
            glTexStorage2D(GL_TEXTURE_2D, mipLevels, GL_RGBA32F, width_, height_);
        }

        // Copy texture data
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, GL_RGBA, GL_FLOAT, bytes);

        stbi_image_free(bytes);

    } else {
        // UInt8 texture
        uint8_t *bytes = stbi_load(filename.c_str(), &width_, &height_, &channels_, STBI_rgb_alpha);
        if (!bytes) {
            FatalError("Failed to load image file: %s", filename.c_str());
        }

        // Inversion
        //imageFlip(bytes, width_, height_, 4);

        // Memory allocation
        if (!generateMipMap) {
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width_, height_);
        } else {
            const int mipLevels = (int)std::ceil(std::log2(std::max(width_, height_)));
            glTexStorage2D(GL_TEXTURE_2D, mipLevels, GL_RGBA8, width_, height_);
        }

        // Copy texture data
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, bytes);

        stbi_image_free(bytes);
    }

    if (generateMipMap) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (generateMipMap) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}
