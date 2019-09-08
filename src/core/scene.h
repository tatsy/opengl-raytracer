#pragma once

#include <string>
#include <vector>
#include <memory>

#include <glm/glm.hpp>

#include "api.h"
#include "uncopyable.h"
#include "trimesh.h"
#include "bvh.h"

namespace glrt {

struct Triangle {
    glm::vec4 indices;  // i, j, k, mtrlID
};

enum class MaterialType : int {
    Emitter = 0x01,
    Diffuse = 0x02,
    Conductor = 0x03,
    Dielectric = 0x04,
    Media = 0x05
};

struct Material {
    glm::vec3 type = glm::vec3(0.0f);
    glm::vec3 emission = glm::vec3(0.0f);
    glm::vec3 param0 = glm::vec3(0.0f);
    glm::vec3 param1 = glm::vec3(0.0f);
    glm::vec3 param2 = glm::vec3(0.0f);
    glm::vec3 texIds = glm::vec3(0.0f);
};

struct VolumeData {
    glm::vec3 bboxMax, bboxMin;
    float maxValue = 0.0f;
    GLuint densityTex = 0u;
    GLuint temperatureTex = 0u;
};

class GLRT_API Scene : private Uncopyable {
public:
    // PUBLIC methods
    Scene();
    Scene(const std::string &filename);

    void parse(const std::string &filename);

private:
    int width, height;
    float apertureRadius, focalLength;
    glm::mat4 modelM, viewM, projM;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<Triangle> triangles;
    std::vector<Triangle> lights;
    std::vector<Material> materials;

    std::shared_ptr<TextureBuffer> vertTexBuffer;
    std::shared_ptr<TextureBuffer> triTexBuffer;
    std::shared_ptr<TextureBuffer> mtrlTexBuffer;
    std::shared_ptr<TextureBuffer> lightTexBuffer;
    std::shared_ptr<TextureBuffer> bvhTexBuffer;

    BVH bvh;

    std::vector<VolumeData> volumes;

    friend class Window;
};

}  // namespace glrt 
