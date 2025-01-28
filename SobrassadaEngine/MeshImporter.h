#pragma once

#include "tiny_gltf.h"
#include "FileSystem.h"
#include "Utils/Globals.h"

#include "Libs/MathGeoLib/include/Math/float3.h"
#include "Libs/MathGeoLib/include/Math/float2.h"
#include <vector>

struct Vertex {
    float position[3];  // Position of the vertex in 3D space
    float normal[3];    // Normal vector for lighting calculations
    float texCoord[2];  // Texture coordinates for mapping textures
};

namespace MeshImporter {

    // Function to import a mesh from a GLTF model
    bool ImportMesh(const tinygltf::Model& model,
        const tinygltf::Mesh& mesh,
        const tinygltf::Primitive& primitive);

}
