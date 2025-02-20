#pragma once

#include <memory>
#include "FileSystem.h"
#include "Mesh.h"
#include "Globals.h"


#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE

#include "ResourceManagement/Resources/ResourceAnimation.h"
#include "tiny_gltf.h"

namespace AnimationImporter
{
    UID ImportAnimation(
        const tinygltf::Model& model, const tinygltf::Animation& animation, const std::string& name,
        const char* filePath
    );
    ResourceAnimation* LoadAnimation(UID animationUID);
}; 