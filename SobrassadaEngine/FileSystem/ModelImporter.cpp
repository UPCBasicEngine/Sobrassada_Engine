#include "ModelImporter.h"

#include "ResourceManagement/Resources/ResourceModel.h"

namespace ModelImporter
{
    UID ImportModel(const std::vector<tinygltf::Node>& nodes)
    {
        // Get Nodes data
        Model newModel;
        std::vector<NodeData> orderedNodes;

        FillNodes(nodes[0], -1, orderedNodes); // -1 parentId for root
      



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

    ResourceModel* LoadModel()
    {
        return nullptr;
    }

    void FillNodes(tinygltf::Node& nodeData, int parentId, std::vector<NodeData>& outNodes)
    {
        // Fill node data
        NodeData newNode;
        newNode.name = nodeData.name;
        //newNode.transform = getTransformSomehow
        newNode.parentId = parentId;
        //newNode.meshes = getMeshesSomehow


        // Call this function for every node child and give them their id, which their children will need
        for (const auto& nodeId : nodeData.children)
        {
            //FillNodes();
        }
    }
}
