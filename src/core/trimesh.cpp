#define GLRT_API_EXPORT
#include "trimesh.h"

#include <iostream>
#include <fstream>
#include <experimental/filesystem>

#include <tiny_obj_loader.h>
#include <tinyply.h>

namespace fs = std::experimental::filesystem;

namespace glrt {

Trimesh::Trimesh() {
}

Trimesh::Trimesh(const std::string &filename) {
    load(filename);
}

void Trimesh::load(const std::string &filename) {
    // Load a new mesh
    std::string extension = fs::path(filename.c_str()).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    bool hasNorm = false;
    bool hasUV = false;
    if (extension == ".obj") {
        loadOBJ(filename, &hasNorm, &hasUV);
    } else if (extension == ".ply") {
        loadPLY(filename, &hasNorm, &hasUV);
    } else {
        FatalError("Unsupported mesh file extension: %s", extension.c_str());
    }

    // Compute normals if they are not given with file
    if (!hasNorm) {
        for (auto &v : vertices) {
            v.normal = glm::vec3(0.0f, 0.0f, 0.0f);
        }

        for (int face = 0; face < indices.size(); face += 3) {
            const glm::vec3 &v0 = vertices[indices[face + 0]].pos;
            const glm::vec3 &v1 = vertices[indices[face + 1]].pos;
            const glm::vec3 &v2 = vertices[indices[face + 2]].pos;
            
            glm::vec3 norm = glm::cross(v1 - v0, v2 - v0);
            const double length = glm::length(norm);
            if (length != 0.0f) {
                norm /= length;
                vertices[indices[face + 0]].normal += norm;
                vertices[indices[face + 1]].normal += norm;
                vertices[indices[face + 2]].normal += norm;
            }
        }

        for (auto &v : vertices) {
            const float l = glm::length(v.normal);
            if (l > 0.0f) {
                v.normal /= l;
            }
        }
    }

    // Compute tangents and binormals using UV coordinates
    if (hasUV) {
        // Initialize tangents and binormals 
        for (auto &v : vertices) {
            v.tangent = glm::vec3(0.0f, 0.0f, 0.0f);
            v.binormal = glm::vec3(0.0f, 0.0f, 0.0f);
        }

        // Compute tangents and binormals
        for (int face = 0; face < indices.size(); face += 3) {
            const glm::vec3 &v0 = vertices[indices[face + 0]].pos;
            const glm::vec3 &v1 = vertices[indices[face + 1]].pos;
            const glm::vec3 &v2 = vertices[indices[face + 2]].pos;
            const glm::vec2 &uv0 = vertices[indices[face + 0]].uv;
            const glm::vec2 &uv1 = vertices[indices[face + 1]].uv;
            const glm::vec2 &uv2 = vertices[indices[face + 2]].uv;

            glm::vec3 deltaPos1 = v1 - v0;
            glm::vec3 deltaPos2 = v2 - v0;
            glm::vec2 deltaUv1 = uv1 - uv0;
            glm::vec2 deltaUv2 = uv2 - uv0;

            float det = deltaUv1.x * deltaUv2.y - deltaUv1.y * deltaUv2.x;
            if (det != 0.0f) {
                glm::vec3 tangent = glm::normalize((-deltaPos1 * deltaUv2.y + deltaPos2 * deltaUv1.y) / det);
                glm::vec3 binormal = glm::normalize((-deltaPos2 * deltaUv1.x + deltaPos1 * deltaUv2.x) / det);

                vertices[indices[face + 0]].tangent += tangent;
                vertices[indices[face + 0]].binormal += binormal;
                vertices[indices[face + 1]].tangent += tangent;
                vertices[indices[face + 1]].binormal += binormal;
                vertices[indices[face + 2]].tangent += tangent;
                vertices[indices[face + 2]].binormal += binormal;
            }
        }

        // Normalize
        for (int i = 0; i < vertices.size(); i++) {
            if (glm::length(vertices[i].tangent) > 0.0 && glm::length(vertices[i].binormal) > 0.0) {
                vertices[i].tangent = glm::normalize(vertices[i].tangent);
                vertices[i].binormal = glm::normalize(vertices[i].binormal);
                //vertices_[i].normal = glm::normalize(glm::cross(vertices_[i].tangent, vertices_[i].binormal));
            }
        }
    }
}

void Trimesh::loadOBJ(const std::string &filename, bool *hasNorm, bool *hasUV) {
    // Resolve mtl file location
    fs::path path(filename);
    const std::string absname = fs::absolute(path).string();
    const std::string dirname = fs::canonical(path.remove_filename()).string() + std::string(1, fs::path::preferred_separator);

    // Open
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> mats;
    std::string errMsg, warnMsg;
    bool success = tinyobj::LoadObj(&attrib, &shapes, &mats, &errMsg, &warnMsg, filename.c_str(), dirname.c_str(), true);
    if (!errMsg.empty()) {
        Warn("%s", errMsg.c_str());
    }

    if (!success) {
        FatalError("Failed to load *.obj file: %s", filename.c_str());
    }

    // Traverse triangles
    std::unordered_map<Vertex, uint32_t> uniqueVertices;
    vertices.clear();
    indices.clear();

    // Each shape
    *hasNorm = true;
    *hasUV = true;
    for (const auto &shape : shapes) {
        // Each index
        const size_t numTriangles = shape.mesh.material_ids.size();
        for (int t = 0; t < numTriangles; t++) {
            for (int i = 0; i < 3; i++) {
                const tinyobj::index_t &index = shape.mesh.indices[t * 3 + i];

                glm::vec3 pos(0.0f);
                glm::vec3 normal(0.0f);
                glm::vec3 uv(0.0f);

                if (index.vertex_index >= 0) {
                    pos = glm::vec3(attrib.vertices[index.vertex_index * 3 + 0],
                                    attrib.vertices[index.vertex_index * 3 + 1],
                                    attrib.vertices[index.vertex_index * 3 + 2]);
                }

                if (index.normal_index >= 0) {
                    normal = glm::vec3(attrib.normals[index.normal_index * 3 + 0],
                                       attrib.normals[index.normal_index * 3 + 1],
                                       attrib.normals[index.normal_index * 3 + 2]);
                } else {
                    *hasNorm = false;
                }

                if (index.texcoord_index >= 0) {
                    uv = glm::vec3(attrib.texcoords[index.texcoord_index * 2 + 0],
                                   attrib.texcoords[index.texcoord_index * 2 + 1],
                                   0.0f);
                } else {
                    *hasUV = false;
                }

                Vertex vertex;
                vertex.pos = pos;
                vertex.normal = normal;
                vertex.uv = uv;

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }
                indices.push_back(uniqueVertices[vertex]);
            }
        }
    }
}

void Trimesh::loadPLY(const std::string &filename, bool *hasNorm, bool *hasUV) {
    using tinyply::PlyFile;
    using tinyply::PlyData;

    *hasNorm = false;
    *hasUV = false;

    try {
        // Open
        std::ifstream reader(filename.c_str(), std::ios::binary);
        if (reader.fail()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }

        // Read header
        PlyFile file;
        file.parse_header(reader);

        // Request vertex data
        std::shared_ptr<PlyData> vert_data, norm_data, uv_data, face_data;
        try {
            vert_data = file.request_properties_from_element("vertex", { "x", "y", "z" });
        } catch (std::exception &e) {
            std::cerr << "tinyply exception: " << e.what() << std::endl;
        }

        try {
            norm_data = file.request_properties_from_element("vertex", { "nx", "ny", "nz" });
        } catch (std::exception &e) {
            std::cerr << "tinyply exception: " << e.what() << std::endl;
        }

        try {
            uv_data = file.request_properties_from_element("vertex", { "u", "v" });
        } catch (std::exception &e) {
            std::cerr << "tinyply exception: " << e.what() << std::endl;
        }

        try {
            face_data = file.request_properties_from_element("face", { "vertex_indices" }, 3);
        } catch (std::exception &e) {
            std::cerr << "tinyply exception: " << e.what() << std::endl;
        }

        // Read vertex data
        file.read(reader);

        // Copy vertex data
        const size_t numVerts = vert_data->count;
        std::vector<float> raw_vertices, raw_normals, raw_uv;
        if (vert_data) {
            raw_vertices.resize(numVerts * 3);
            std::memcpy(raw_vertices.data(), vert_data->buffer.get(), sizeof(float) * numVerts * 3);
        }

        if (norm_data) {
            *hasNorm = true;
            raw_normals.resize(numVerts * 3);
            std::memcpy(raw_normals.data(), norm_data->buffer.get(), sizeof(float) * numVerts * 3);
        }

        if (uv_data) {
            *hasUV = true;
            raw_uv.resize(numVerts * 2);
            std::memcpy(raw_uv.data(), uv_data->buffer.get(), sizeof(float) * numVerts * 3);
        }

        const size_t numFaces = face_data->count;
        std::vector<uint32_t> raw_indices(numFaces * 3);
        std::memcpy(raw_indices.data(), face_data->buffer.get(), sizeof(uint32_t) * numFaces * 3);

        std::unordered_map<Vertex, uint32_t> uniqueVertices;
        vertices.clear();
        for (uint32_t i : raw_indices) {
            glm::vec3 pos;
            glm::vec3 normal;
            glm::vec3 uv;

            if (vert_data) {
                pos = glm::vec3(raw_vertices[i * 3 + 0],
                                raw_vertices[i * 3 + 1],
                                raw_vertices[i * 3 + 2]);
            }

            if (norm_data) {
                normal = glm::vec3(raw_normals[i * 3 + 0],
                                   raw_normals[i * 3 + 1],
                                   raw_normals[i * 3 + 2]);
            }

            if (uv_data) {
                uv = glm::vec3(raw_uv[i * 2 + 0],
                               raw_uv[i * 2 + 1],
                               0.0f);
            }

            Vertex vertex;
            vertex.pos = pos;
            vertex.normal = normal;
            vertex.uv = uv;

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }
            indices.push_back(uniqueVertices[vertex]);
        }
    } catch (const std::exception &e) {
        std::cerr << "Caught tinyply exception: " << e.what() << std::endl;
    }
}


}  // namespace glrt
