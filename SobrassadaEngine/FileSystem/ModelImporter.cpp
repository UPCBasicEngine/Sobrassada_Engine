#include "ModelImporter.h"

#include "Application.h"
#include "LibraryModule.h"

#include "Math/Quat.h"
#include "Math/float4x4.h"
#include "ResourceManagement/Resources/ResourceModel.h"

#include "prettywriter.h"
#include "stringbuffer.h"

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

        // for (int i = 0; i < orderedNodes.size(); ++i)
        //{
        //     GLOG(
        //         "Node %d. Name: %s. Parent id: %d. Meshes count: %d", i, orderedNodes[i].name.c_str(),
        //         orderedNodes[i].parentId, orderedNodes[i].meshes.size()
        //     )
        // }

        newModel.SetNodes(orderedNodes);

        // Save in JSON format, way easier for this data
        // Create doc JSON
        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

        rapidjson::Value modelJSON(rapidjson::kObjectType);

        UID modelUID         = GenerateUID();
        std::string savePath = MODELS_PATH + std::string("Model") + MODEL_EXTENSION;
        UID finalModelUID    = App->GetLibraryModule()->AssignFiletypeUID(modelUID, savePath);
        savePath             = MODELS_PATH + std::to_string(finalModelUID) + MODEL_EXTENSION;
        std::string name     = FileSystem::GetFileNameWithoutExtension(filePath);
        newModel.SetUID(finalModelUID);

        // Create structure
        modelJSON.AddMember("UID", finalModelUID, allocator);

        // Serialize ordered nodes
        rapidjson::Value nodesJSON(rapidjson::kArrayType);

        for (const NodeData& node : orderedNodes)
        {
            rapidjson::Value nodeDataJSON(rapidjson::kObjectType);

            nodeDataJSON.AddMember("Name", rapidjson::Value(node.name.c_str(), allocator), allocator);

            rapidjson::Value valTransform(rapidjson::kArrayType);
            valTransform.PushBack(node.transform.position.x, allocator)
                .PushBack(node.transform.position.y, allocator)
                .PushBack(node.transform.position.z, allocator)
                .PushBack(node.transform.rotation.x, allocator)
                .PushBack(node.transform.rotation.y, allocator)
                .PushBack(node.transform.rotation.z, allocator)
                .PushBack(node.transform.scale.x, allocator)
                .PushBack(node.transform.scale.y, allocator)
                .PushBack(node.transform.scale.z, allocator);

            nodeDataJSON.AddMember("Transform", valTransform, allocator);

            nodeDataJSON.AddMember("ParentIndex", node.parentIndex, allocator);

            // Save mesh and material UID in same array
            if (node.meshes.size() > 0)
            {
                rapidjson::Value valMeshes(rapidjson::kArrayType);
                for (const std::pair<UID, UID>& ids : node.meshes)
                {
                    valMeshes.PushBack(ids.first, allocator);
                    valMeshes.PushBack(ids.second, allocator);
                }
                nodeDataJSON.AddMember("MeshesMaterials", valMeshes, allocator);
            }
            nodesJSON.PushBack(nodeDataJSON, allocator);
        }

        modelJSON.AddMember("Nodes", nodesJSON, allocator);
        doc.AddMember("Model", modelJSON, allocator);

        // Save file like JSON
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        unsigned int bytesWritten =
            (unsigned int)FileSystem::Save(savePath.c_str(), buffer.GetString(), (unsigned int)buffer.GetSize(), false);
        if (bytesWritten == 0)
        {
            GLOG("Failed to save model file: %s", savePath);
            return 0;
        }

        App->GetLibraryModule()->AddModel(finalModelUID, name);
        App->GetLibraryModule()->AddResource(savePath, finalModelUID);

        GLOG("%s saved as model", name.c_str());

        // Testing
        // LoadModel(finalModelUID);

        return finalModelUID;

        // unsigned int size = sizeof(Model);
        // char* fileBuffer  = new char[size];
        // memcpy(fileBuffer, &newModel, size);
        // unsigned int bytesWritten = (unsigned int)FileSystem::Save(savePath.c_str(), fileBuffer, size, true);
        //
        // delete[] fileBuffer;

        // if (bytesWritten == 0)
        //{
        //     GLOG("Failed to save model: %s", savePath.c_str());
        //     return 0;
        // }
    }

    ResourceModel* LoadModel(UID modelUID)
    {
        rapidjson::Document doc;
        const std::string filePath = App->GetLibraryModule()->GetResourcePath(modelUID);

        FileSystem::LoadJSON(filePath.c_str(), doc);

        if (!doc.HasMember("Model") || !doc["Model"].IsObject())
        {
            GLOG("Invalid model format: %s", filePath);
            return nullptr;
        }

        rapidjson::Value& modelJSON = doc["Model"];

        // Scene values
        UID uid                     = modelJSON["UID"].GetUint64();

        std::vector<NodeData> loadedNodes;

        // Deserialize Nodes
        if (modelJSON.HasMember("Nodes") && modelJSON["Nodes"].IsArray())
        {
            const rapidjson::Value& nodesJSON = modelJSON["Nodes"];

            for (rapidjson::SizeType i = 0; i < nodesJSON.Size(); i++)
            {
                const rapidjson::Value& nodeJSON = nodesJSON[i];

                NodeData newNode;
                newNode.name = nodeJSON["Name"].GetString();

                if (nodeJSON.HasMember("Transform") && nodeJSON["Transform"].IsArray() &&
                    nodeJSON["Transform"].Size() == 9)
                {
                    const rapidjson::Value& initLocalTransform = nodeJSON["Transform"];

                    newNode.transform                          = Transform(
                        float3(
                            initLocalTransform[0].GetFloat(), initLocalTransform[1].GetFloat(),
                            initLocalTransform[2].GetFloat()
                        ),
                        float3(
                            initLocalTransform[3].GetFloat(), initLocalTransform[4].GetFloat(),
                            initLocalTransform[5].GetFloat()
                        ),
                        float3(
                            initLocalTransform[6].GetFloat(), initLocalTransform[7].GetFloat(),
                            initLocalTransform[8].GetFloat()
                        )
                    );
                }

                newNode.parentIndex = nodeJSON["ParentIndex"].GetInt();

                if (nodeJSON.HasMember("MeshesMaterials") && nodeJSON["MeshesMaterials"].IsArray())
                {
                    const rapidjson::Value& uids = nodeJSON["MeshesMaterials"];

                    for (rapidjson::SizeType i = 0; i < uids.Size(); i += 2)
                    {
                        newNode.meshes.emplace_back(uids[i].GetUint64(), uids[i + 1].GetUint64());
                    }
                }

                loadedNodes.push_back(newNode);
            }
        }

        ResourceModel* resourceModel = new ResourceModel(uid, FileSystem::GetFileNameWithoutExtension(filePath));
        resourceModel->SetModelData(Model(uid, loadedNodes));

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
        newNode.name               = nodeData.name;

        float4x4 rawTransform      = GetNodeTransform(nodeData);
        Transform parentTransform  = Transform::identity;
        if (parentId != -1) parentTransform = outNodes[parentId].transform;
        newNode.transform.position = parentTransform.position + rawTransform.TranslatePart();
        newNode.transform.rotation = parentTransform.rotation + rawTransform.RotatePart().ToEulerXYZ();
        newNode.parentIndex        = parentId;

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

    const float4x4& GetNodeTransform(const tinygltf::Node& node)
    {

        if (!node.matrix.empty())
        {
            // glTF stores matrices in COLUMN-MAJOR order, same as MathGeoLib
            return float4x4(
                node.matrix[0], node.matrix[1], node.matrix[2], node.matrix[3], node.matrix[4], node.matrix[5],
                node.matrix[6], node.matrix[7], node.matrix[8], node.matrix[9], node.matrix[10], node.matrix[11],
                node.matrix[12], node.matrix[13], node.matrix[14], node.matrix[15]
            );
        }

        // Default values
        float3 translation = float3::zero;
        Quat rotation      = Quat::identity;
        float3 scale       = float3::one;

        if (!node.translation.empty())
            translation = float3(node.translation[0], node.translation[1], node.translation[2]);

        if (!node.rotation.empty())
            rotation = Quat(
                node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]
            ); // glTF stores as [x, y, z, w]

        if (!node.scale.empty()) scale = float3(node.scale[0], node.scale[1], node.scale[2]);

        return float4x4::FromTRS(translation, rotation, scale);
    }
} // namespace ModelImporter
