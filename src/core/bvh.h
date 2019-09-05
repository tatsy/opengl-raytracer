#pragma once

#include <vector>

#include "api.h"
#include "common.h"
#include "trimesh.h"

namespace glrt {

struct Bounds {
    Bounds()
        : posMin(1.0e8, 1.0e8, 1.0e8)
        , posMax(-1.0e8, -1.0e8, -1.0e8) {
    }

    void merge(const glm::vec3& v) {
        posMin.x = std::min(posMin.x, v.x);
        posMax.x = std::max(posMax.x, v.x);
        posMin.y = std::min(posMin.y, v.y);
        posMax.y = std::max(posMax.y, v.y);
        posMin.z = std::min(posMin.z, v.z);
        posMax.z = std::max(posMax.z, v.z);
    }

    static Bounds merge(const Bounds& b0, const Bounds& b1) {
        Bounds b;
        b.posMin.x = std::min(b0.posMin.x, b1.posMin.x);
        b.posMax.x = std::max(b0.posMax.x, b1.posMax.x);
        b.posMin.y = std::min(b0.posMin.y, b1.posMin.y);
        b.posMax.y = std::max(b0.posMax.y, b1.posMax.y);
        b.posMin.z = std::min(b0.posMin.z, b1.posMin.z);
        b.posMax.z = std::max(b0.posMax.z, b1.posMax.z);
        return b;
    }

    int maxExtent() const {
        const float xSpan = std::abs(posMax.x - posMin.x);
        const float ySpan = std::abs(posMax.y - posMin.y);
        const float zSpan = std::abs(posMax.z - posMin.z);
        const float maxSpan = std::max(xSpan, std::max(ySpan, zSpan));
        if (maxSpan == xSpan) return 0;
        if (maxSpan == ySpan) return 1;
        return 2;
    }

    float area() const {
        const float xSpan = std::abs(posMax.x - posMin.x);
        const float ySpan = std::abs(posMax.y - posMin.y);
        const float zSpan = std::abs(posMax.z - posMin.z);
        return 2.0f * (xSpan * ySpan + ySpan * zSpan + zSpan * xSpan);
    }

    glm::vec3 posMin;
    glm::vec3 posMax;
};

struct TriangleInfo {
    TriangleInfo() {}
    TriangleInfo(int i, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2) {
        index = i;
        v[0] = v0;
        v[1] = v1;
        v[2] = v2;

        bounds.posMin.x = std::min(v[0].x, std::min(v[1].x, v[2].x));
        bounds.posMax.x = std::max(v[0].x, std::max(v[1].x, v[2].x));
        bounds.posMin.y = std::min(v[0].y, std::min(v[1].y, v[2].y));
        bounds.posMax.y = std::max(v[0].y, std::max(v[1].y, v[2].y));
        bounds.posMin.z = std::min(v[0].z, std::min(v[1].z, v[2].z));
        bounds.posMax.z = std::max(v[0].z, std::max(v[1].z, v[2].z));

        centroid.x = (v0.x + v1.x + v2.x) / 3.0f;
        centroid.y = (v0.y + v1.y + v2.y) / 3.0f;
        centroid.z = (v0.z + v1.z + v2.z) / 3.0f;
    }

    int index;
    glm::vec3 v[3];
    glm::vec3 centroid;
    Bounds bounds;
};

struct BVHNode {
    void initLeaf(const Bounds& b, int index) {
        bboxMin = b.posMin;
        bboxMax = b.posMax;
        children = glm::vec3(-1.0f, -1.0f, index);
    }

    void initFork(const Bounds &b, int left, int right, int axis) {
        bboxMin = b.posMin;
        bboxMax = b.posMax;
        children = glm::vec3(left, right, -1.0f);
    }

    glm::vec3 bboxMin;
    glm::vec3 bboxMax;
    glm::vec3 children;  // left, right, triangle
};

struct GLRT_API BVH {
    BVH();
    explicit BVH(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);
    virtual ~BVH();

    void construct(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);
    int constructRec(std::vector<TriangleInfo> &prims, int left, int right);

    std::vector<BVHNode> nodes;
};

}  // namespace glrt
