#pragma once

#include "FileSystem.h"
#include "Globals.h"

#include "Math/float3.h"
#include "Math/float2.h"
#include <vector>
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "tiny_gltf.h"

struct Vertex {
    float position[3];  // Position of the vertex in 3D space
    float normal[3];    // Normal vector for lighting calculations
    float texCoord[2];  // Texture coordinates for mapping textures
};

namespace MeshImporter {

    // Function to import a mesh from a GLTF model
    bool ImportMesh(const tinygltf::Model& model,
        const tinygltf::Mesh& mesh,
        const tinygltf::Primitive& primitive,
        const std::string& filePath);

};