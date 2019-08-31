#pragma once

#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include "api.h"
#include "common.h"

struct Vertex {
    bool operator==(const Vertex& v) const {
        return pos == v.pos && normal == v.normal && uv == v.uv;
    }

    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 uv;  // use 3D for TBO
    glm::vec3 tangent = glm::vec3(0.0f);
    glm::vec3 binormal = glm::vec3(0.0f);
};

namespace std {

template <>
struct hash<Vertex> {
    size_t operator()(const Vertex &v) const {
        size_t h = 0;
        h = hash<glm::vec3>()(v.pos) ^ (h << 1);
        h = hash<glm::vec3>()(v.normal) ^ (h << 1);
        h = hash<glm::vec2>()(v.uv) ^ (h << 1);
        return h;
    }
};

}  // namespace std

namespace glrt {

class GLRT_API Trimesh {
public:
    Trimesh();
    Trimesh(const std::string &filename);
    void load(const std::string &filename);

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

private:
    void loadOBJ(const std::string &filename, bool *hasNorm, bool *hasUV);
    void loadPLY(const std::string &filename, bool *hasNorm, bool *hasUV);
};

}  // namespace glrt
