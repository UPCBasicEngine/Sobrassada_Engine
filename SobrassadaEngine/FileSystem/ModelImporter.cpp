#include "ModelImporter.h"

#include "Application.h"
#include "LibraryModule.h"

#include "ResourceManagement/Resources/ResourceModel.h"

namespace ModelImporter
{
    UID ImportModel(
        const std::vector<tinygltf::Node>& nodes, const std::vector<std::vector<std::pair<UID, UID>>>& meshesUIDs,
        const char* filePath
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
                orderedNodes[i].parentId, orderedNodes[i].meshes.size()
            )
        }

        newModel.SetNodes(orderedNodes);

        // Save

        UID modelUID         = GenerateUID();

        std::string savePath = MODELS_PATH + std::string("Material") + MODEL_EXTENSION;
        UID finalModelUID    = App->GetLibraryModule()->AssignFiletypeUID(modelUID, savePath);
        std::string fileName = FileSystem::GetFileNameWithoutExtension(filePath);
        savePath             = MODELS_PATH + std::to_string(finalModelUID) + MODEL_EXTENSION;

        newModel.SetUID(finalModelUID);
        unsigned int size = sizeof(Model);
        char* fileBuffer  = new char[size];
        memcpy(fileBuffer, &newModel, size);
        unsigned int bytesWritten = (unsigned int)FileSystem::Save(savePath.c_str(), fileBuffer, size, true);

        delete[] fileBuffer;

        if (bytesWritten == 0)
        {
            GLOG("Failed to save model: %s", savePath.c_str());
            return 0;
        }

        // App->GetLibraryModule()->AddMaterial(finalMaterialUID, materialName);
        App->GetLibraryModule()->AddResource(savePath, finalModelUID);

        GLOG("%s saved as model", savePath.c_str());

        // Testing
        LoadModel(finalModelUID);

        return finalModelUID;
    }

    ResourceModel* LoadModel(UID modelUID)
    {
        char* buffer     = nullptr;
        std::string path = App->GetLibraryModule()->GetResourcePath(modelUID);
        GLOG("Model path: %s", path.c_str());

        unsigned int fileSize = FileSystem::Load(path.c_str(), &buffer);

        if (fileSize == 0 || buffer == nullptr)
        {
            GLOG("Failed to load the .model file: ");
            return nullptr;
        }

        char* cursor                 = buffer;

        // Create Mesh
        Model model                  = *reinterpret_cast<Model*>(cursor);

        ResourceModel* resourceModel = new ResourceModel(modelUID, FileSystem::GetFileNameWithoutExtension(path));

        // material->LoadMaterialData(mat);

        delete[] buffer;

        GLOG("Loaded model with %d nodes:", model.GetNodesCount());

        const std::vector<NodeData>& nodes = model.GetNodes();
        for (int i = 0; i < model.GetNodesCount(); ++i)
        {
            GLOG(
                "Node %d. Name: %s. Parent id: %d. Meshes count: %d", i, nodes[i].name.c_str(), nodes[i].parentId,
                nodes[i].meshes.size()
            )
        }

        // App->GetLibraryModule()->AddResource(savePath, finalMeshUID);

        return resourceModel;
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
