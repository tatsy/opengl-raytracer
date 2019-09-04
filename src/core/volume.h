#pragma once

#include <cstdio>
#include <cstring>
#include <string>

#include <glm/glm.hpp>

#include "api.h"

namespace glrt {

struct GLRT_API Volume {
    Volume();

    Volume(int size_x, int size_y, int size_z, int channels);

    Volume(const Volume &vol);

    virtual ~Volume();

    Volume &operator=(const Volume &vol);

    float &operator()(int x, int y, int z, int ch) {
        return data[((z * size_y + y) * size_x + x) * channels + ch];
    }

    void release() {
        if (data) {
            delete[] data;
            data = nullptr;
        }
    }

    void range(const glm::vec3 &bboxMin, const glm::vec3 &bboxMax);

    void resize(int size_x, int size_y, int size_z, int channels);

    void load(const std::string &filename);

    void save(const std::string &filename) const;

    int size_x, size_y, size_z;
    int channels;
    glm::vec3 bboxMin;
    glm::vec3 bboxMax;
    float maxValue = 0.0f;
    float *data = nullptr;
};

}  // namespace glrt
