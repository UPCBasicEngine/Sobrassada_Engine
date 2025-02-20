#include "ModelImporter.h"

#include "ResourceManagement/Resources/ResourceModel.h"

namespace ModelImporter
{
    UID ImportModel(
        const std::vector<tinygltf::Node>& nodes, const std::vector<std::vector<std::pair<UID, UID>>>& meshesUIDs
    )
    {
        // Get Nodes data
        Model newModel;
        std::vector<NodeData> orderedNodes;

        GLOG("Start filling nodes")
        FillNodes(nodes, 0, -1, meshesUIDs, orderedNodes); // -1 parentId for root
        GLOG("Nodes filled");

        for (int i = 0; i < orderedNodes.size(); ++i)
        {
            GLOG(
                "Node %d. Name: %s. Parent id: %d. Meshes count: %d", i, orderedNodes[i].name.c_str(),
                orderedNodes[i].parentId,
                orderedNodes[i].meshes.size()
            )
        }

        // Save
        /*
        UID materialUID      = GenerateUID();

        std::string savePath = MATERIALS_PATH + std::string("Material") + MATERIAL_EXTENSION;
        UID finalMaterialUID = App->GetLibraryModule()->AssignFiletypeUID(materialUID, savePath);
        std::string fileName = FileSystem::GetFileNameWithoutExtension(filePath);

        savePath             = MATERIALS_PATH + std::to_string(finalMaterialUID) + MATERIAL_EXTENSION;

        material.SetMaterialUID(finalMaterialUID);
        unsigned int size = sizeof(Material);
        char* fileBuffer  = new char[size];
        memcpy(fileBuffer, &material, sizeof(Material));
        unsigned int bytesWritten = (unsigned int)FileSystem::Save(savePath.c_str(), fileBuffer, size, true);

        delete[] fileBuffer;

        if (bytesWritten == 0)
        {
            GLOG("Failed to save material: %s", savePath.c_str());
            return 0;
        }

        App->GetLibraryModule()->AddMaterial(finalMaterialUID, materialName);
        App->GetLibraryModule()->AddResource(savePath, finalMaterialUID);

        GLOG("%s saved as material", materialName.c_str());

        return finalMaterialUID;
        */
        return 0;
    }

    ResourceModel* LoadModel(UID modelUID)
    {
        return nullptr;
    }

    void FillNodes(
        const std::vector<tinygltf::Node>& nodesList, int nodeId, int parentId,
        const std::vector<std::vector<std::pair<UID, UID>>>& meshesUIDs, std::vector<NodeData>& outNodes
    )
    {
        // Fill node data
        const tinygltf::Node& nodeData = nodesList[nodeId];
        NodeData newNode;
        newNode.name     = nodeData.name;

        // newNode.transform = getTransformSomehow
        newNode.parentId = parentId;

        // Get reference to Mesh and Material UIDs
        if (nodeData.mesh > -1)
        {
            newNode.meshes = meshesUIDs[nodeData.mesh];
        }

        outNodes.push_back(newNode);

        // Call this function for every node child and give them their id, which their children will need
        for (const auto& id : nodeData.children)
        {
            FillNodes(nodesList, id, nodeId, meshesUIDs, outNodes);
        }
    }
} // namespace ModelImporter
