#define GLRT_API_EXPORT
#include "volume.h"

#include <cstdio>
#include <algorithm>
#include <stdexcept>

namespace glrt {

Volume::Volume() {
}

Volume::Volume(int size_x, int size_y, int size_z, int channels)
    : size_x(size_x), size_y(size_y), size_z(size_z), channels(channels) {
    data = new float[size_x * size_y * size_z * channels];
    std::memset(data, 0, sizeof(float) * size_x * size_y * size_z * channels);
}

Volume::Volume(const Volume &vol)
    : Volume{} {
    this->operator=(vol);
}

Volume::~Volume() {
    release();
}

Volume &Volume::operator=(const Volume &vol) {
    this->size_x = vol.size_x;
    this->size_y = vol.size_y;
    this->size_z = vol.size_z;

    this->bboxMin = vol.bboxMin;
    this->bboxMax = vol.bboxMax;

    this->channels = vol.channels;

    release();
    data = new float[size_x * size_y * size_z];
    std::memcpy(data, vol.data, sizeof(float) * size_x * size_y * size_z * channels);

    return *this;
}

void Volume::range(const glm::vec3 &bboxMin, const glm::vec3 &bboxMax) {
    this->bboxMin = bboxMin;
    this->bboxMax = bboxMax;
}

void Volume::resize(int size_x, int size_y, int size_z, int channels) {
    release();
    this->size_x = size_x;
    this->size_y = size_y;
    this->size_z = size_z;
    this->channels = channels;
    data = new float[size_x * size_y * size_z * channels];
    std::memset(data, 0, sizeof(float) * size_x * size_y * size_z * channels);
}

void Volume::load(const std::string &filename) {
    FILE *fp = fopen(filename.c_str(), "rb");
    if (!fp) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    char header[4] = {0};
    fread(header, sizeof(char), 3, fp);
    if (std::strcmp(header, "VOL") != 0) {
        throw std::runtime_error("Invalid indetifier: " + std::string(header));
    }

    char version;
    fread(&version, sizeof(char), 1, fp);
    if (version != 3) {
        char msg[256];
        sprintf(msg, "Invalid version: %d", (int) version);
        throw std::runtime_error(std::string(msg));
    }

    int identifier;
    fread(&identifier, sizeof(int), 1, fp);
    if (identifier != 1) {
        throw std::runtime_error("Currently support only float32 types!\n");
    }

    fread(&size_x, sizeof(int), 1, fp);
    fread(&size_y, sizeof(int), 1, fp);
    fread(&size_z, sizeof(int), 1, fp);
    fread(&channels, sizeof(int), 1, fp);

    fread(&bboxMin, sizeof(float), 3, fp);
    fread(&bboxMax, sizeof(float), 3, fp);

    release();
    data = new float[size_x * size_y * size_z * channels];
    fread(data, sizeof(float), size_x * size_y * size_z * channels, fp);

    maxValue = -1.0e10f;
    for (int i = 0; i < size_x * size_y * size_z * channels; i++) {
        maxValue = std::max(maxValue, data[i]);
    }

    fclose(fp);
}

void Volume::save(const std::string &filename) const {
    FILE *fp = fopen(filename.c_str(), "wb");
    if (!fp) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    const char *header = "VOL";
    fwrite(header, sizeof(char), 3, fp);

    const char version = 3;
    fwrite(&version, sizeof(char), 1, fp);

    const int identifier = 1;
    fwrite(&identifier, sizeof(int), 1, fp);

    fwrite(&size_x, sizeof(int), 1, fp);
    fwrite(&size_y, sizeof(int), 1, fp);
    fwrite(&size_z, sizeof(int), 1, fp);
    fwrite(&channels, sizeof(int), 1, fp);

    fwrite(&bboxMin, sizeof(float), 3, fp);
    fwrite(&bboxMax, sizeof(float), 3, fp);

    fwrite(data, sizeof(float), size_x * size_y * size_z * channels, fp);

    fclose(fp);
}

}  // namespace glrt
