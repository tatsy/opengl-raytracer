#pragma once

#include <string>
#include <vector>
#include <memory>

#include <glm/glm.hpp>

#include "api.h"
#include "uncopyable.h"
#include "trimesh.h"

namespace glrt {

struct BVHNode {
    glm::vec3 bboxMin;
    glm::vec3 bboxMax;
    glm::vec3 children;
};

struct Triangle {
    glm::vec4 indices;  // i, j, k, mtrlID
};

struct Material {
    glm::vec3 E = glm::vec3(0.0f);
    glm::vec3 Kd = glm::vec3(0.0f);
};

class GLRT_API Scene : private Uncopyable {
public:
    // PUBLIC methods
    Scene();
    Scene(const std::string &filename);

    void parse(const std::string &filename);

private:
    int width, height;
    glm::mat4 modelM, viewM, projM;

    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;
    std::vector<Triangle> lights;
    std::vector<Material> materials;

    GLuint vertTexId, vertBufId;
    GLuint triTexId, triBufId;
    GLuint matTexId, matBufId;
    GLuint lightTexId, lightBufId;

    friend class Window;
};

}  // namespace glrt 
