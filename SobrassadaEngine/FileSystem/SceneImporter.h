#pragma once

#include "Math/float4x4.h"


#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "tiny_gltf.h"

namespace SceneImporter
{
    void Import(const char *filePath);
    void ImportGLTF(const char *filePath);
    void CreateLibraryDirectories();

    float4x4 GetNodeTransform(const tinygltf::Node& node);
}; // namespace SceneImporter