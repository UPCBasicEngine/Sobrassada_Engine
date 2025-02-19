#pragma once

#include "FileSystem.h"
#include "Mesh.h"
#include "Globals.h"
#include <memory>

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "tiny_gltf.h"
#include "ResourceManagement/Resources/ResourceMesh.h"

enum DataType
{
    UNSIGNED_CHAR,
    UNSIGNED_SHORT,
    UNSIGNED_INT
}; 

namespace MeshImporter
{

    // Function to import a mesh from a GLTF model
    UID ImportMesh(
        const tinygltf::Model &model, const tinygltf::Mesh &mesh, const tinygltf::Primitive &primitive,
        const std::string &name, const char * filePath, float4x4 transform
    );
    ResourceMesh* LoadMesh(UID meshUID);
}; 
