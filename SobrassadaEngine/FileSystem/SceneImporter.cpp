#include "SceneImporter.h"

#include "FileSystem.h"
#include "Globals.h"
#include "MaterialImporter.h"
#include "MeshImporter.h"
#include "TextureImporter.h"


#include "Math/Quat.h"

namespace SceneImporter
{
    void Import(const char* filePath)
    {
        CreateLibraryDirectories();

        std::string extension = FileSystem::GetFileExtension(filePath);

        if (extension == ASSET_EXTENSION) ImportGLTF(filePath);
        else TextureImporter::Import(filePath);
    }

    void ImportGLTF(const char* filePath)
    {
        // Copy gltf to Assets folder
        {
            std::string copyPath = ASSETS_PATH + FileSystem::GetFileNameWithExtension(filePath);

            if (!FileSystem::Exists(copyPath.c_str()))
            {
                FileSystem::Copy(filePath, copyPath.c_str());
            }
        }

        tinygltf::TinyGLTF gltfContext;
        tinygltf::Model model;
        std::string err, warn, name;
        int n    = 0;

        bool ret = gltfContext.LoadASCIIFromFile(&model, &err, &warn, std::string(filePath));
        if (!ret)
        {
            if (!warn.empty())
            {
                GLOG("Warn: %s\n", warn.c_str());
            }

            if (!err.empty())
            {
                GLOG("Err: %s\n", err.c_str());
            }

            if (!ret)
            {
                GLOG("Failed to parse glTF\n");
            }
        }

        std::string path = FileSystem::GetFilePath(filePath);
        std::vector<float4x4> nodeTransforms(model.nodes.size(), float4x4::identity);

        for (size_t i = 0; i < model.nodes.size(); ++i)
        {
            const tinygltf::Node& node = model.nodes[i];

            float4x4 localTransform    = float4x4::identity;

            localTransform          = GetNodeTransform(node);

            nodeTransforms[i]          = nodeTransforms[i] * localTransform;
            //multiply parent by all children
            if (node.children.size() > 0)
            {
                for (size_t j = 0; j < node.children.size(); ++j)
                {
                    nodeTransforms[node.children[j]] = nodeTransforms[i] * nodeTransforms[node.children[j]];
                }
            }

            if (node.mesh >= 0)
            {

                int meshnode               = node.mesh;
                const tinygltf::Mesh& mesh = model.meshes[node.mesh];
                int n                      = 0;
                int matIndex               = 0;

                for (const auto& primitive : mesh.primitives)
                {
                    matIndex             = primitive.material;
                    std::string meshName = mesh.name + std::to_string(n);
                    MeshImporter::ImportMesh(model, mesh, primitive, meshName, filePath, nodeTransforms[i]);
                    MaterialImporter::ImportMaterial(model, matIndex, filePath);
                    n++;
                }
            }
        }

        // Compute world transform
        // float4x4 worldTransform    = ComputeWorldTransform(model, i, parentTransform);
        // nodeTransforms[i]          = worldTransform;

        // Copy bin to Assets folder
        for (const auto& srcBuffers : model.buffers)
        {
            std::string binPath  = path + srcBuffers.uri;
            std::string copyPath = ASSETS_PATH + FileSystem::GetFileNameWithExtension(binPath);
            if (!FileSystem::Exists(copyPath.c_str()))
            {
                FileSystem::Copy(binPath.c_str(), copyPath.c_str());
            }
        }
    }

    void CreateLibraryDirectories()
    {
        if (!FileSystem::IsDirectory(ASSETS_PATH))
        {
            if (!FileSystem::CreateDirectories(ASSETS_PATH))
            {
                GLOG("Failed to create directory: %s", ASSETS_PATH);
            }
        }
        if (!FileSystem::IsDirectory(ANIMATIONS_PATH))
        {
            if (!FileSystem::CreateDirectories(ANIMATIONS_PATH))
            {
                GLOG("Failed to create directory: %s", ANIMATIONS_PATH);
            }
        }
        if (!FileSystem::IsDirectory(AUDIO_PATH))
        {
            if (!FileSystem::CreateDirectories(AUDIO_PATH))
            {
                GLOG("Failed to create directory: %s", AUDIO_PATH);
            }
        }
        if (!FileSystem::IsDirectory(BONES_PATH))
        {
            if (!FileSystem::CreateDirectories(BONES_PATH))
            {
                GLOG("Failed to create directory: %s", BONES_PATH);
            }
        }
        if (!FileSystem::IsDirectory(MESHES_PATH))
        {
            if (!FileSystem::CreateDirectories(MESHES_PATH))
            {
                GLOG("Failed to create directory: %s", MESHES_PATH);
            }
        }
        if (!FileSystem::IsDirectory(TEXTURES_PATH))
        {
            if (!FileSystem::CreateDirectories(TEXTURES_PATH))
            {
                GLOG("Failed to create directory: %s", TEXTURES_PATH);
            }
        }
        if (!FileSystem::IsDirectory(MATERIALS_PATH))
        {
            if (!FileSystem::CreateDirectories(MATERIALS_PATH))
            {
                GLOG("Failed to create directory: %s", MATERIALS_PATH);
            }
        }
        if (!FileSystem::IsDirectory(SCENES_PATH))
        {
            if (!FileSystem::CreateDirectories(SCENES_PATH))
            {
                GLOG("Failed to create directory: %s", SCENES_PATH);
            }
        }
    }


    float4x4 GetNodeTransform(const tinygltf::Node& node)
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
}; // namespace SceneImporter