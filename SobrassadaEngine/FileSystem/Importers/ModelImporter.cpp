#include "ModelImporter.h"

#include "Application.h"
#include "FileSystem.h"
#include "LibraryModule.h"
#include "MeshImporter.h"
#include "MetaModel.h"
#include "Model.h"
#include "ProjectModule.h"
#include "ResourceModel.h"
#include "AnimationImporter.h"
#include "Math/Quat.h"
#include "Math/float4x4.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "tiny_gltf.h"

namespace ModelImporter
{
    UID ImportModel(
        const tinygltf::Model& model, const std::vector<std::vector<std::pair<UID, UID>>>& meshesUIDs,
        const char* filePath, const std::string& targetFilePath, const UID sourceUID
    )
    {
        // Get Nodes data
        Model newModel; // needed?

        std::vector<int> rootNodes;
        std::vector<NodeData> allNodes;
        allNodes.resize(model.nodes.size());

        for (const auto& scene : model.scenes)
        {
            for (const auto& nodeID : scene.nodes)
            {
                FillNodes(model.nodes, nodeID, meshesUIDs, allNodes);
                rootNodes.push_back(nodeID);
            }
        }

        newModel.SetRootNodesIdx(rootNodes);
        newModel.SetNodes(allNodes);

        // Get Skins data
        std::vector<Skin> skinsData;
        for (const tinygltf::Skin& skin : model.skins)
        {
            Skin newSkin;
            newSkin.rootIndex    = skin.skeleton;
            newSkin.bonesIndices = skin.joints;

            if (skin.inverseBindMatrices != -1)
            {
                std::vector<float4x4> inverseBindMatrices;

                const tinygltf::Accessor& matAcc      = model.accessors[skin.inverseBindMatrices];
                const tinygltf::BufferView& matView   = model.bufferViews[matAcc.bufferView];
                const tinygltf::Buffer& matBufferData = model.buffers[matView.buffer];
                const unsigned char* bufferMatrices   = &(matBufferData.data[matAcc.byteOffset + matView.byteOffset]);
                size_t stride = (matView.byteStride > 0) ? matView.byteStride : sizeof(float4x4);

                for (size_t i = 0; i < matAcc.count; ++i)
                {
                    const float4x4 matrix  = *reinterpret_cast<const float4x4*>(bufferMatrices);
                    bufferMatrices        += stride;
                    inverseBindMatrices.push_back(matrix.Transposed());
                    //GLOG("I: %d. %f, %f, %f, %f", i, matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3]);
                }
                //GLOG("Bind matrices length: %d", inverseBindMatrices.size());

                newSkin.inverseBindMatrices = inverseBindMatrices;
            }

            skinsData.push_back(newSkin);
        }

        // Save in JSON format, way easier for this data
        // Create doc JSON
        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

        rapidjson::Value modelJSON(rapidjson::kObjectType);

        const std::string& modelName = FileSystem::GetFileNameWithoutExtension(filePath);
        std::string assetPath        = MODELS_ASSETS_PATH + modelName + MODEL_EXTENSION;
        UID finalModelUID;
        if (sourceUID == INVALID_UID)
        {
            UID modelUID  = GenerateUID();
            modelUID = App->GetLibraryModule()->AssignFiletypeUID(modelUID, FileType::Model);

            UID prefix                 = modelUID / UID_PREFIX_DIVISOR;
            const std::string savePath = App->GetProjectModule()->GetLoadedProjectPath() + METADATA_PATH +
                                         std::to_string(prefix) + FILENAME_SEPARATOR + modelName + META_EXTENSION;
            finalModelUID = App->GetLibraryModule()->GetUIDFromMetaFile(savePath);
            if (finalModelUID == INVALID_UID) finalModelUID = modelUID;
            
            MetaModel meta(finalModelUID, assetPath);
            meta.Save(modelName, assetPath);
        }
        else finalModelUID = sourceUID;

        assetPath = targetFilePath + assetPath;
        std::vector<UID> animationUIDs;
        // Import Animations
        if (model.animations.size() > 0)
        {

            //GLOG("Number of animations in model: %zu", model.animations.size());

            for (int i = 0; i < model.animations.size(); ++i)
            {
                const auto& anim = model.animations[i];

                //GLOG("Importing animation %d", i);
                //GLOG("Animation channels: %zu", anim.channels.size());

                UID animUID =
                    AnimationImporter::ImportAnimation(model, anim, anim.name, filePath, targetFilePath, sourceUID, i);

                //GLOG("Imported animation UID: %llu", animUID);

                if (animUID != INVALID_UID)
                {
                    animationUIDs.push_back(animUID);

                    //GLOG("Final animation UID: %llu", animUID);
                }

                else
                {
                    GLOG("Failed to import animation %d", i);
                }
            }

            if (!animationUIDs.empty())
            {
                newModel.SetAnimationUID(animationUIDs[0]);
                newModel.SetAllAnimationUIDs(animationUIDs);

                //GLOG("Set first animation UID: %llu", animationUIDs[0]);
                //GLOG("Total animation UIDs: %zu", animationUIDs.size());
            }
            else
            {
                GLOG("No valid animation UIDs found");
            }
        }

        newModel.SetUID(finalModelUID);

        // Create structure
        modelJSON.AddMember("UID", finalModelUID, allocator);

        rapidjson::Value scenesJSON(rapidjson::kArrayType);
        for (const unsigned int rootNode : rootNodes)
        {
            rapidjson::Value valRootNode(rapidjson::kObjectType);
            valRootNode.AddMember("rootNode", rootNode, allocator);
            scenesJSON.PushBack(valRootNode, allocator);
        }
        modelJSON.AddMember("Scenes", scenesJSON, allocator);

        // Serialize ordered nodes
        rapidjson::Value nodesJSON(rapidjson::kArrayType);
        for (const NodeData& node : allNodes)
        {
            rapidjson::Value nodeDataJSON(rapidjson::kObjectType);
            nodeDataJSON.AddMember("Name", rapidjson::Value(node.name.c_str(), allocator), allocator);

            rapidjson::Value valTransform(rapidjson::kArrayType);
            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    valTransform.PushBack(node.transform[i][j], allocator);
                }
            }

            nodeDataJSON.AddMember("Transform", valTransform, allocator);
            nodeDataJSON.AddMember("ParentIndex", node.parentIndex, allocator);

            rapidjson::Value valChildren(rapidjson::kArrayType);
            for (int i = 0; i < node.children.size(); ++i)
            {
                valChildren.PushBack(node.children[i], allocator);
            }
            nodeDataJSON.AddMember("Children", valChildren, allocator);

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

            if (node.skinIndex != -1) nodeDataJSON.AddMember("SkinIndex", node.skinIndex, allocator);

            nodesJSON.PushBack(nodeDataJSON, allocator);
        }

        modelJSON.AddMember("Nodes", nodesJSON, allocator);
        // Serialize Animations.
        rapidjson::Value animationsJSON(rapidjson::kArrayType);
        for (UID animUID : animationUIDs)
        {
            animationsJSON.PushBack(animUID, allocator);
        }
        modelJSON.AddMember("Animations", animationsJSON, allocator);
        // Serialize skins
        rapidjson::Value skinsJSON(rapidjson::kArrayType);
        for (const Skin& skin : skinsData)
        {
            rapidjson::Value skinDataJSON(rapidjson::kObjectType);

            skinDataJSON.AddMember("RootIndex", skin.rootIndex, allocator);

            rapidjson::Value valBones(rapidjson::kArrayType);
            for (const int bone : skin.bonesIndices)
            {
                valBones.PushBack(bone, allocator);
            }
            skinDataJSON.AddMember("BonesIndices", valBones, allocator);

            rapidjson::Value valMatrices(rapidjson::kArrayType);
            for (const float4x4& matrix : skin.inverseBindMatrices)
            {
                rapidjson::Value valMatrix(rapidjson::kArrayType);
                for (int i = 0; i < 4; ++i)
                {
                    for (int j = 0; j < 4; ++j)
                    {
                        valMatrix.PushBack(matrix[i][j], allocator);
                    }
                }
                valMatrices.PushBack(valMatrix, allocator);
            }
            skinDataJSON.AddMember("InverseBindMatrices", valMatrices, allocator);

            skinsJSON.PushBack(skinDataJSON, allocator);
        }
        modelJSON.AddMember("Skins", skinsJSON, allocator);

        doc.AddMember("Model", modelJSON, allocator);
       
        // Save file like JSON
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        std::string saveFilePath = App->GetProjectModule()->GetLoadedProjectPath() + MODELS_LIB_PATH +
                                   std::to_string(finalModelUID) + MODEL_EXTENSION;
        unsigned int bytesWritten = (unsigned int
        )FileSystem::Save(saveFilePath.c_str(), buffer.GetString(), (unsigned int)buffer.GetSize(), false);
        if (bytesWritten == 0)
        {
            GLOG("Failed to save model file: %s", saveFilePath);
            return 0;
        }

        if (sourceUID == INVALID_UID)
        {
            unsigned int bytesWritten = (unsigned int
            )FileSystem::Save(assetPath.c_str(), buffer.GetString(), (unsigned int)buffer.GetSize(), false);
            if (bytesWritten == 0)
            {
                GLOG("Failed to save model file: %s", assetPath);
                return 0;
            }
        }

        App->GetLibraryModule()->AddModel(finalModelUID, modelName);
        App->GetLibraryModule()->AddName(modelName, finalModelUID);
        App->GetLibraryModule()->AddResource(saveFilePath, finalModelUID);

        //GLOG("%s saved as model", modelName.c_str());

        return finalModelUID;
    }

    void CopyModel(
        const std::string& filePath, const std::string& targetFilePath, const std::string& name, const UID sourceUID
    )
    {
        std::string destination = targetFilePath + MODELS_LIB_PATH + std::to_string(sourceUID) + MODEL_EXTENSION;
        FileSystem::Copy(filePath.c_str(), destination.c_str());

        App->GetLibraryModule()->AddModel(sourceUID, name);
        App->GetLibraryModule()->AddName(name, sourceUID);
        App->GetLibraryModule()->AddResource(destination, sourceUID);
    }

    ResourceModel* LoadModel(UID modelUID)
    {
        rapidjson::Document doc;
        const std::string& filePath = App->GetLibraryModule()->GetResourcePath(modelUID);

        if (!FileSystem::LoadJSON(filePath.c_str(), doc)) return nullptr;

        if (!doc.HasMember("Model") || !doc["Model"].IsObject())
        {
            GLOG("Invalid model format: %s", filePath);
            return nullptr;
        }

        rapidjson::Value& modelJSON = doc["Model"];

        // Scene values
        UID uid                     = modelJSON["UID"].GetUint64();

        std::vector<int> rootNodesIdx;
        if (modelJSON.HasMember("Scenes") && modelJSON["Scenes"].IsArray())
        {
            const rapidjson::Value& scenesJSON = modelJSON["Scenes"];
            for (rapidjson::SizeType i = 0; i < scenesJSON.Size(); i++)
            {
                const rapidjson::Value& sceneJSON = scenesJSON[i];
                rootNodesIdx.push_back(sceneJSON["rootNode"].GetInt());
            }
        }

        std::vector<NodeData> loadedNodes;
        if (modelJSON.HasMember("Nodes") && modelJSON["Nodes"].IsArray())
        {
            const rapidjson::Value& nodesJSON = modelJSON["Nodes"];
            for (rapidjson::SizeType i = 0; i < nodesJSON.Size(); i++)
            {
                const rapidjson::Value& nodeJSON = nodesJSON[i];

                NodeData newNode;
                newNode.name = nodeJSON["Name"].GetString();

                if (nodeJSON.HasMember("Transform") && nodeJSON["Transform"].IsArray() &&
                    nodeJSON["Transform"].Size() == 16)
                {
                    const rapidjson::Value& initLocalTransform = nodeJSON["Transform"];
                    int counter                                = 0;
                    for (int i = 0; i < 4; ++i)
                    {
                        for (int j = 0; j < 4; ++j)
                        {
                            newNode.transform[i][j] = initLocalTransform[counter].GetFloat();
                            ++counter;
                        }
                    }
                }

                newNode.parentIndex = nodeJSON["ParentIndex"].GetInt();

                if (nodeJSON.HasMember("Children") && nodeJSON["Children"].IsArray())
                {
                    const rapidjson::Value& childrenNodes = nodeJSON["Children"];
                    for (rapidjson::SizeType i = 0; i < childrenNodes.Size(); i++)
                    {
                        newNode.children.push_back(childrenNodes[i].GetInt());
                    }
                }

                if (nodeJSON.HasMember("MeshesMaterials") && nodeJSON["MeshesMaterials"].IsArray())
                {
                    const rapidjson::Value& uids = nodeJSON["MeshesMaterials"];

                    for (rapidjson::SizeType i = 0; i < uids.Size(); i += 2)
                    {
                        newNode.meshes.emplace_back(uids[i].GetUint64(), uids[i + 1].GetUint64());
                    }
                }

                if (nodeJSON.HasMember("SkinIndex") && nodeJSON["SkinIndex"].IsInt())
                {
                    newNode.skinIndex = nodeJSON["SkinIndex"].GetInt();
                }
                else
                {
                    newNode.skinIndex = -1;
                }

                loadedNodes.push_back(newNode);
            }
        }

         std::vector<UID> loadedAnimations;
        // Deserialize Animations
        if (modelJSON.HasMember("Animations") && modelJSON["Animations"].IsArray())
        {

            const rapidjson::Value& animJSON = modelJSON["Animations"];

            for (rapidjson::SizeType i = 0; i < animJSON.Size(); ++i)
            {

                loadedAnimations.push_back(animJSON[i].GetUint64());
            }
        }


        // Deserialize Skins
        std::vector<Skin> loadedSkins;
        if (modelJSON.HasMember("Skins") && modelJSON["Skins"].IsArray())
        {
            const rapidjson::Value& skinsJSON = modelJSON["Skins"];
            for (rapidjson::SizeType i = 0; i < skinsJSON.Size(); i++)
            {
                const rapidjson::Value& skinJSON = skinsJSON[i];

                Skin newSkin;
                newSkin.rootIndex = skinJSON["RootIndex"].GetInt();

                if (skinJSON.HasMember("BonesIndices") && skinJSON["BonesIndices"].IsArray())
                {
                    const rapidjson::Value& initBonesIndices = skinJSON["BonesIndices"];
                    for (unsigned int i = 0; i < initBonesIndices.Size(); ++i)
                    {
                        newSkin.bonesIndices.push_back(initBonesIndices[i].GetInt());
                    }
                }

                if (skinJSON.HasMember("InverseBindMatrices") && skinJSON["InverseBindMatrices"].IsArray())
                {
                    const rapidjson::Value& initMatrices = skinJSON["InverseBindMatrices"];
                    for (unsigned int i = 0; i < initMatrices.Size(); ++i)
                    {
                        if (initMatrices[i].IsArray() && initMatrices[i].Size() == 16)
                        {
                            const rapidjson::Value& initMatrix = initMatrices[i];
                            float4x4 matrix                    = float4x4::identity;
                            int pos                            = 0;
                            for (int i = 0; i < 4; ++i)
                            {
                                for (int j = 0; j < 4; ++j)
                                {
                                    matrix[i][j] = initMatrix[pos].GetFloat();
                                    ++pos;
                                }
                            }
                            newSkin.inverseBindMatrices.push_back(matrix);
                        }
                    }
                }

                loadedSkins.push_back(newSkin);
                /*GLOG(
                    "Skin bones count: %d. Matrices count: %d", newSkin.bonesIndices.size(),
                    newSkin.inverseBindMatrices.size()
                );*/
            }
        }

        ResourceModel* resourceModel = new ResourceModel(uid, FileSystem::GetFileNameWithoutExtension(filePath));
        resourceModel->SetModelData(Model(uid, rootNodesIdx, loadedNodes, loadedSkins));
        if (loadedAnimations.size() > 0)
        {
            resourceModel->SetAllAnimationUIDs(loadedAnimations);
            resourceModel->SetAnimationUID(loadedAnimations[0]);
        }

        return resourceModel;
    }

    void FillNodes(
        const std::vector<tinygltf::Node>& nodesList, int nodeID,
        const std::vector<std::vector<std::pair<UID, UID>>>& meshesUIDs, std::vector<NodeData>& outNodes
    )
    {
        std::vector<NodeParent> nodesToVisit;
        nodesToVisit.push_back({nodeID, -1}); // root parentID = -1

        while (!nodesToVisit.empty())
        {
            NodeParent currentNode = nodesToVisit.back();
            nodesToVisit.pop_back();

            const int currentNodeIndex     = currentNode.nodeID;
            const tinygltf::Node& nodeData = nodesList[currentNodeIndex];

            NodeData newNode;
            if (!nodeData.name.empty()) newNode.name = nodeData.name;
            else newNode.name = DEFAULT_NODE_NAME;

            if (nodeData.mesh != -1) newNode.transform = float4x4::identity;
            else newNode.transform = MeshImporter::GetNodeTransform(nodeData);

            newNode.parentIndex = currentNode.parentID;
            newNode.children    = nodeData.children;

            // Get reference to Mesh and Material UIDs
            if (nodeData.mesh > -1) newNode.meshes = meshesUIDs[nodeData.mesh];

            newNode.skinIndex          = nodeData.skin;

            outNodes[currentNodeIndex] = newNode;

            for (auto it = nodeData.children.rbegin(); it != nodeData.children.rend(); ++it)
            {
                nodesToVisit.push_back({*it, currentNodeIndex});
            }
        }
    }
} // namespace ModelImporter