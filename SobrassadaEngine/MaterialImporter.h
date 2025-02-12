#pragma once

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "tiny_gltf.h"

namespace MaterialImporter
{
    bool ImportMaterial(const tinygltf::Model &model, int materialIndex, const std::string &filePath);
};
