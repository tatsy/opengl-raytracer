#define GLRT_API_EXPORT
#include "scene.h"

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#include <glm/gtx/hash.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tiny_obj_loader.h>
#include <json11/json11.hpp>
using namespace json11;

#include "common.h"
#include "texture.h"
#include "texture_buffer.h"
#include "volume.h"

namespace glrt {

// ---------------------------------------------------------------------------------------------------------------------
// PUBLIC methods
// ---------------------------------------------------------------------------------------------------------------------

Scene::Scene() {}

Scene::Scene(const std::string &filename) { parse(filename); }

void Scene::parse(const std::string &filename) {
    // Get all file contents
    std::ifstream reader(filename.c_str(), std::ios::in);
    if (reader.fail()) {
        FatalError("Failed to open file: %s", filename.c_str());
    }

    std::string jsonText;
    reader.seekg(0, std::ios::end);
    jsonText.reserve(reader.tellg());
    reader.seekg(std::ios::beg);
    jsonText.assign(std::istreambuf_iterator<char>(reader), std::istreambuf_iterator<char>());
    reader.close();

    // Parse JSON text
    std::string err;
    const auto json = Json::parse(jsonText, err);
    if (!err.empty()) {
        Warn("%s", err.c_str());
    }

    // Base directory
    fs::path fpath(filename.c_str());
    const fs::path baseDirPath = fs::absolute(fpath).parent_path();
    
    // Film
    width = json["film"]["width"].int_value();
    height = json["film"]["height"].int_value();
    Info("window: %d x %d", width, height);

    // Camera
    const std::string type = json["camera"]["type"].string_value();
    Info("Camera type: %s", type.c_str());
    if (type == "perspective") {
        // Model matrix
        modelM = glm::mat4(1.0f);

        // View matrix
        const glm::vec3 origin = glm::vec3(json["camera"]["lookAt"]["origin"][0].number_value(),
                                           json["camera"]["lookAt"]["origin"][1].number_value(),
                                           json["camera"]["lookAt"]["origin"][2].number_value());
        const glm::vec3 target = glm::vec3(json["camera"]["lookAt"]["target"][0].number_value(),
                                           json["camera"]["lookAt"]["target"][1].number_value(),
                                           json["camera"]["lookAt"]["target"][2].number_value());
        const glm::vec3 up = glm::vec3(json["camera"]["lookAt"]["up"][0].number_value(),
                                       json["camera"]["lookAt"]["up"][1].number_value(),
                                       json["camera"]["lookAt"]["up"][2].number_value());
        viewM = glm::lookAt(origin, target, up);

        // Projection matrix
        const float fov = (float)json["camera"]["fov"].number_value();
        const float aspect = (float)width / (float)height;
        const float zNear = (float)json["camera"]["nearClip"].number_value();
        const float zFar = (float)json["camera"]["farClip"].number_value();
        projM = glm::perspective(glm::radians(fov), aspect, zNear, zFar);
    }

    // Shapes
    vertices.clear();
    indices.clear();
    triangles.clear();
    lights.clear();
    materials.clear();
    const auto &shapes = json["scene"].array_items();
    for (int i = 0; i < shapes.size(); i++) {
        // Material
        const std::string material = shapes[i]["material"].string_value();
        Material mtrl;
        if (material == "diffuse") {
            mtrl.Kd = glm::vec3(shapes[i]["reflectance"][0].number_value(),
                                shapes[i]["reflectance"][1].number_value(),
                                shapes[i]["reflectance"][2].number_value());
        } else if (material == "emitter") {
            mtrl.E = glm::vec3(shapes[i]["emission"][0].number_value(),
                               shapes[i]["emission"][1].number_value(),
                               shapes[i]["emission"][2].number_value());
        } else if(material == "volume") {
            const int baseIndex = volumes.size();
            VolumeData voldata;
            {
                const std::string &filename = shapes[i]["volume"]["density"].string_value();
                Volume volume;
                volume.load((baseDirPath / fs::path(filename.c_str())).string());

                GLuint texId;
                glGenTextures(1, &texId);
                glBindTexture(GL_TEXTURE_3D, texId);
                glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32F, volume.size_x, volume.size_y, volume.size_z);
                glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, volume.size_x, volume.size_y, volume.size_z, GL_RED,
                                GL_FLOAT, volume.data);

                voldata.densityTex = texId;
                voldata.maxValue = volume.maxValue;
                voldata.bboxMin = glm::vec3(shapes[i]["volume"]["bboxMin"][0].number_value(),
                                            shapes[i]["volume"]["bboxMin"][1].number_value(),
                                            shapes[i]["volume"]["bboxMin"][2].number_value());
                voldata.bboxMax = glm::vec3(shapes[i]["volume"]["bboxMax"][0].number_value(),
                                            shapes[i]["volume"]["bboxMax"][1].number_value(),
                                            shapes[i]["volume"]["bboxMax"][2].number_value());
            }

            {
                const std::string &filename = shapes[i]["volume"]["temperature"].string_value();
                Volume volume;
                volume.load((baseDirPath / fs::path(filename.c_str())).string());

                GLuint texId;
                glGenTextures(1, &texId);
                glBindTexture(GL_TEXTURE_3D, texId);
                glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32F, volume.size_x, volume.size_y, volume.size_z);
                glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, volume.size_x, volume.size_y, volume.size_z, GL_RED,
                                GL_FLOAT, volume.data);

                voldata.temperatureTex = texId;
            }
            volumes.push_back(voldata);
            mtrl.tex = glm::vec3(baseIndex, baseIndex + 1, 0.0);
        } else {
            FatalError("Unsupported material: %s", material.c_str());
        }
        materials.push_back(mtrl);

        // Triangles
        const std::string type = shapes[i]["type"].string_value();
        if (type == "obj") {
            const std::string &objfile = shapes[i]["filename"].string_value();
            Trimesh mesh((baseDirPath / fs::path(objfile.c_str())).string());

            const int baseIndex = (int)vertices.size();
            const int nVerts = mesh.vertices.size();
            for (int i = 0; i < nVerts; i++) {
                vertices.push_back(mesh.vertices[i]);
            }

            const int nTris = mesh.indices.size() / 3;
            for (int i = 0; i < nTris; i++) {
                Triangle tri;
                tri.indices.x = baseIndex + mesh.indices[i * 3 + 0];
                tri.indices.y = baseIndex + mesh.indices[i * 3 + 1];
                tri.indices.z = baseIndex + mesh.indices[i * 3 + 2];
                tri.indices.w = materials.size() - 1;
                triangles.push_back(tri);

                indices.push_back(baseIndex + mesh.indices[i * 3 + 0]);
                indices.push_back(baseIndex + mesh.indices[i * 3 + 1]);
                indices.push_back(baseIndex + mesh.indices[i * 3 + 2]);

                if (glm::length(mtrl.E) != 0.0f) {
                    lights.push_back(tri);
                }
            }
        }
    }

    // Construct BVH
    bvh.construct(vertices, indices);
    bvhTexBuffer = std::make_shared<TextureBuffer>(bvh.nodes.size() * sizeof(BVHNode), GL_RGB32F, GL_STATIC_DRAW);
    bvhTexBuffer->setData(bvh.nodes.data());

    // Transfer to OpenGL
    vertTexBuffer = std::make_shared<TextureBuffer>(vertices.size() * sizeof(Vertex), GL_RGB32F, GL_STATIC_DRAW);
    vertTexBuffer->setData(vertices.data());

    triTexBuffer = std::make_shared<TextureBuffer>(triangles.size() * sizeof(Triangle), GL_RGBA32F, GL_STATIC_DRAW);
    triTexBuffer->setData(triangles.data());

    mtrlTexBuffer = std::make_shared<TextureBuffer>(materials.size() * sizeof(Material), GL_RGB32F, GL_STATIC_DRAW);
    mtrlTexBuffer->setData(materials.data());

    lightTexBuffer = std::make_shared<TextureBuffer>(lights.size() * sizeof(Triangle), GL_RGBA32F, GL_STATIC_DRAW);
    lightTexBuffer->setData(lights.data());

    Info("Scene setup OK!\n");
    Info("#vertex: %d", (int)vertices.size());
    Info("#triangle: %d", (int)triangles.size());
    Info("#BVH noede: %d", (int)bvh.nodes.size());
}

// ---------------------------------------------------------------------------------------------------------------------
// PRIVATE methods
// ---------------------------------------------------------------------------------------------------------------------

}  // namespace glrt
