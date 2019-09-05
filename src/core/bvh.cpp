#define GLRT_API_EXPORT
#include "bvh.h"

namespace glrt {

struct ComparePoint {
    explicit ComparePoint(int axis)
        : axis(axis) {
    }

    bool operator()(const TriangleInfo& t0, const TriangleInfo& t1) const {
        return t0.centroid[axis] < t1.centroid[axis];
    }

    int axis;
};

struct BucketInfo {
    int count;
    Bounds bounds;
    BucketInfo()
        : count(0)
        , bounds() {
    }
};

struct CompareToBucket {
    int splitBucket, nBuckets, dim;
    const Bounds& centroidBounds;

    CompareToBucket(int split, int num, int d, const Bounds& b)
        : splitBucket(split)
        , nBuckets(num)
        , dim(d)
        , centroidBounds(b) {
    }

    bool operator()(const TriangleInfo& p) const {
        const double cmin = centroidBounds.posMin[dim];
        const double cmax = centroidBounds.posMax[dim];
        const double inv = (1.0) / (std::abs(cmax - cmin) + 1.0e-8);
        const double diff = std::abs(p.centroid[dim] - cmin);
        int b = static_cast<int>(nBuckets * diff * inv);
        if (b >= nBuckets) {
            b = nBuckets - 1;
        }
        return b <= splitBucket;
    }
};

BVH::BVH() {}

BVH::BVH(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
    construct(vertices, indices);
}

BVH::~BVH() {}

void BVH::construct(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
    std::vector<TriangleInfo> prims;
    const int nTris = (int)indices.size() / 3;
    for (int i = 0; i < nTris; i++) {
        const glm::vec3 v0 = vertices[indices[i * 3 + 0]].pos;
        const glm::vec3 v1 = vertices[indices[i * 3 + 1]].pos;
        const glm::vec3 v2 = vertices[indices[i * 3 + 2]].pos;
        prims.emplace_back(i, v0, v1, v2);
    }

    constructRec(prims, 0, prims.size());
}

int BVH::constructRec(std::vector<TriangleInfo> &prims, int left, int right) {
    if (left == right) {
        return -1;
    }

    const int nodeId = static_cast<int>(nodes.size());
    nodes.push_back(BVHNode());

    Bounds bounds;
    for (int i = left; i < right; i++) {
        bounds = Bounds::merge(bounds, prims[i].bounds);
    }

    int nprims = right - left;
    if (nprims == 1) {
        // Leaf node
        nodes[nodeId].initLeaf(bounds, prims[left].index);
    } else {
        // Fork node
        Bounds centroidBounds;
        for (int i = left; i < right; i++) {
            centroidBounds.merge(prims[i].centroid);
        }

        int splitAxis = centroidBounds.maxExtent();
        int mid = (left + right) / 2;
        if (nprims <= 8) {
            std::nth_element(prims.begin() + left,
                             prims.begin() + mid,
                             prims.begin() + right,
                             ComparePoint(splitAxis));
        } else {
            // Seperate with SAH (surface area heuristics)
            const int nBuckets = 16;
            BucketInfo buckets[nBuckets];

            const double cmin = centroidBounds.posMin[splitAxis];
            const double cmax = centroidBounds.posMax[splitAxis];
            const double idenom = 1.0 / (std::abs(cmax - cmin) + 1.0e-8);
            for (int i = left; i < right; i++) {
                const double numer = prims[i].centroid[splitAxis] - centroidBounds.posMin[splitAxis];
                int b = static_cast<int>(nBuckets * std::abs(numer) * idenom);
                if (b == nBuckets) {
                    b = nBuckets - 1;
                }

                buckets[b].count++;
                buckets[b].bounds = Bounds::merge(buckets[b].bounds, prims[i].bounds);
            }

            double bucketCost[nBuckets - 1] = {0};
            for (int i = 0; i < nBuckets - 1; i++) {
                Bounds b0, b1;
                int cnt0 = 0, cnt1 = 0;
                for (int j = 0; j <= i; j++) {
                    b0 = Bounds::merge(b0, buckets[j].bounds);
                    cnt0 += buckets[j].count;
                }
                for (int j = i + 1; j < nBuckets; j++) {
                    b1 = Bounds::merge(b1, buckets[j].bounds);
                    cnt1 += buckets[j].count;
                }
                bucketCost[i] += 0.125 + (cnt0 * b0.area() + cnt1 * b1.area()) / bounds.area();
            }

            double minCost = bucketCost[0];
            int minCostSplit = 0;
            for (int i = 1; i < nBuckets - 1; i++) {
                if (minCost > bucketCost[i]) {
                    minCost = bucketCost[i];
                    minCostSplit = i;
                }
            }

            if (minCost < nprims) {
                auto it = std::partition(prims.begin() + left,
                                         prims.begin() + right,
                                         CompareToBucket(minCostSplit, nBuckets, splitAxis, centroidBounds));
                mid = it - prims.begin();
            }
        }

        const int leftChild  = constructRec(prims, left, mid);
        const int rightChild = constructRec(prims, mid, right);
        nodes[nodeId].initFork(bounds, leftChild, rightChild, splitAxis);
    }

    return nodeId;
}

}  // namespace glrt
