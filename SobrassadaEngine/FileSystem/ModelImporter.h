#pragma once

#include "FileSystem.h"

#include "tiny_gltf.h"

class ResourceModel;

namespace ModelImporter
{
    UID ImportModel(const std::vector<tinygltf::Node>& nodes);
    ResourceModel* LoadModel(UID modelUID);
};
