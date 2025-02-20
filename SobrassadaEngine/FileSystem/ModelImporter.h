#pragma once

#include "FileSystem.h"
#include "Model.h"

#include "tiny_gltf.h"

class ResourceModel;

namespace ModelImporter
{
    UID ImportModel(
        const std::vector<tinygltf::Node>& nodes, const std::vector<std::vector<std::pair<UID, UID>>>& meshesUIDs,
        const char* filePath
    );
    ResourceModel* LoadModel(UID modelUID);
    void FillNodes(
        const std::vector<tinygltf::Node>& nodesList, int nodeId, int parentId,
        const std::vector<std::vector<std::pair<UID, UID>>>& meshesUIDs, std::vector<NodeData>& outNodes
    );
}; // namespace ModelImporter
