#include "SceneImporter.h"

#include "FileSystem.h"
#include "Globals.h"
#include "Application.h"
#include "MeshImporter.h"
#include "TextureImporter.h"
#include "MaterialImporter.h"
#include "LibraryModule.h"
#include "Algorithm/Random/LCG.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "tiny_gltf.h"

namespace SceneImporter
{
    void Import(const char *filePath)
    {
        CreateLibraryDirectories();

        std::string extension = FileSystem::GetFileExtension(filePath);

        if (extension == ASSET_EXTENSION) ImportGLTF(filePath);
        else TextureImporter::Import(filePath);
    }

    void ImportGLTF(const char *filePath)
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

        // Copy bin to Assets folder
        for (const auto &srcBuffers : model.buffers)
        {
            std::string binPath  = path + srcBuffers.uri;
            std::string copyPath = ASSETS_PATH + FileSystem::GetFileNameWithExtension(binPath);
            if (!FileSystem::Exists(copyPath.c_str()))
            {
                FileSystem::Copy(binPath.c_str(), copyPath.c_str());
            }
        }

        for (const auto &srcImages : model.images)
        {
            std::string fullPath = path + srcImages.uri;

            std::string ddsPath  = TextureImporter::Import(fullPath.c_str());
            //mapping dds path to texture name(png)
            if (!ddsPath.empty())
            {
                App->GetLibraryModule()->AddTexture(srcImages.uri, ddsPath);
            }  
        }

        int matIndex = 0;
        for (const auto &srcMaterials : model.materials)
        {
            std::string materialName = srcMaterials.name;
            std::string materialPath = MATERIALS_PATH + materialName + MATERIAL_EXTENSION;

            MaterialImporter::ImportMaterial(model, matIndex, materialName);
            matIndex++;
        }

        for (const auto &srcMesh : model.meshes)
        {
            name = srcMesh.name;
            n    = 0;
            for (const auto &primitive : srcMesh.primitives)
            {
                name += std::to_string(n);
                
                uint64_t uuid = GenerateUUID();
                name += "_" + std::to_string(uuid);

                MeshImporter::ImportMesh(model, srcMesh, primitive, name);
                n++;
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

    uint64_t GenerateUUID() {
        // Set a 64-bit seed (choose any nonzero value)
        uint32_t seed = 0x12345678;

        // Construct the LCG with the seed.
        LCG rng;
        rng.Seed(seed);

        // Generate a 64-bit UUID using two calls to Int(), as the LCG generates 32-bit numbers.
        uint64_t uuid = static_cast<uint64_t>(rng.Int()) << 32 | rng.Int();  // Combine two 32-bit values

        return uuid;
    }
};