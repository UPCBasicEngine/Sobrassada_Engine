#pragma once

#include "FileSystem.h"
#include "Mesh.h"
#include <memory>

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "tiny_gltf.h"

namespace MeshImporter
{

    // Function to import a mesh from a GLTF model
    bool ImportMesh(
        const tinygltf::Model &model, const tinygltf::Mesh &mesh, const tinygltf::Primitive &primitive,
        const std::string &filePath
    );
    std::shared_ptr<Mesh> LoadMesh(const char *path);
}; // namespace MeshImporter

//save two first digits of uid to identify material or mesh 00 01