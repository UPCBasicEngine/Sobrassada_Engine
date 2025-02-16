#include "MaterialImporter.h"

#include "Application.h"
#include "FileSystem.h"
#include "LibraryModule.h"
#include "Material.h"
#include "TextureImporter.h"
#include <FileSystem>

UID MaterialImporter::ImportMaterial(const tinygltf::Model &model, int materialIndex, const char * filePath)
{
    std::string path = FileSystem::GetFilePath(filePath);
    const tinygltf::Material &gltfMaterial = model.materials[materialIndex];
    const std::string materialName = gltfMaterial.name;

    auto it                                = gltfMaterial.extensions.find("KHR_materials_pbrSpecularGlossiness");
    //ADD OLD LOADING

    Material material;
    if (it != gltfMaterial.extensions.end())
    {
        
        const tinygltf::Value &specGloss    = it->second;

        // Diffuse Factor
        const tinygltf::Value &diffuseValue = specGloss.Get("diffuseFactor");
        if (diffuseValue.IsArray() && diffuseValue.ArrayLen() == 4)
        {
            const std::vector<tinygltf::Value> &diffuseArray = diffuseValue.Get<tinygltf::Value::Array>();

            float4 diffuseFactor(
                static_cast<float>(diffuseArray[0].Get<double>()), static_cast<float>(diffuseArray[1].Get<double>()),
                static_cast<float>(diffuseArray[2].Get<double>()), static_cast<float>(diffuseArray[3].Get<double>())
            );

            material.SetDiffuseFactor(diffuseFactor);
        }

        // Diffuse Texture
        if (specGloss.Has("diffuseTexture"))
        {
            // need a pointer to corresponding texture from textureImporter

            const tinygltf::Value &diffuseTex = specGloss.Get("diffuseTexture");
            int texIndex                      = diffuseTex.Get("index").Get<int>();

            UID diffuseUID =
                TextureImporter::Import((path + model.images[model.textures[texIndex].source].uri).c_str());

            if (diffuseUID != CONSTANT_EMPTY_UID)
            {
                material.SetDiffuseTexture(diffuseUID);
            }
        }

        if (specGloss.Has("glossinessFactor"))
        {
            material.SetGlossinessFactor(static_cast<float>(specGloss.Get("glossinessFactor").Get<double>()));
        }

        // Specular Factor
        if (specGloss.Has("specularFactor"))
        {
            const std::vector<tinygltf::Value> &specArray = diffuseValue.Get<tinygltf::Value::Array>();

            if (specArray.size() > 3)
            {
                float3 specular                               = {
                    static_cast<float>(specArray[0].Get<double>()), static_cast<float>(specArray[1].Get<double>()),
                    static_cast<float>(specArray[2].Get<double>())
                };

                material.SetSpecularFactor(specular);
            }
        }

        // Specular-Glossiness Texture
        if (specGloss.Has("specularGlossinessTexture"))
        {
            const tinygltf::Value &specTex = specGloss.Get("specularGlossinessTexture");
            int texIndex                   = specTex.Get("index").Get<int>();

            UID specularGlossinessUID = TextureImporter::Import((path + model.images[model.textures[texIndex].source].uri).c_str()
            );
            if (specularGlossinessUID != CONSTANT_EMPTY_UID)
            {
                material.SetSpecularGlossinessTexture(specularGlossinessUID);
            }
        }
    }

    // Normal Map
    if (gltfMaterial.normalTexture.index >= 0)
    {
        int texIndex = gltfMaterial.normalTexture.index;

        UID normalUID = TextureImporter::Import((path + model.images[model.textures[texIndex].source].uri).c_str()
            );
        if (normalUID != CONSTANT_EMPTY_UID)
        {
            material.SetNormalTexture(normalUID);
        }
    }


    if (gltfMaterial.occlusionTexture.index >= 0)
    {
        int texIndex = gltfMaterial.occlusionTexture.index;

        material.SetOcclusionStrength(static_cast<float>(gltfMaterial.occlusionTexture.strength));

        UID occlusionUID = TextureImporter::Import((path + model.images[model.textures[texIndex].source].uri).c_str()
            );
        if (occlusionUID != CONSTANT_EMPTY_UID)
        {
            material.SetOcclusionTexture(occlusionUID);
        }
    }

    UID materialUID           = GenerateUID();
        
    std::string savePath = MATERIALS_PATH + std::string("Material") + MATERIAL_EXTENSION;
    UID finalMaterialUID = App->GetLibraryModule()->AssignFiletypeUID(materialUID, savePath);
    std::string fileName = FileSystem::GetFileNameWithoutExtension(filePath);

    savePath = MATERIALS_PATH + std::to_string(finalMaterialUID) + MATERIAL_EXTENSION;
    
    material.SetMaterialUID(finalMaterialUID);
    unsigned int size = sizeof(Material);
    char *fileBuffer = new char[size];
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
}

ResourceMaterial* MaterialImporter::LoadMaterial(UID materialUID)
    {
        char* buffer          = nullptr;

        std::string path      = App->GetLibraryModule()->GetResourcePath(materialUID);

        unsigned int fileSize = FileSystem::Load(path.c_str(), &buffer);

        if (fileSize == 0 || buffer == nullptr)
        {
            GLOG("Failed to load the .mat file: ");
            return nullptr;
        }

        char* cursor = buffer;

        // Create Mesh
        Material mat               = *reinterpret_cast<Material*>(cursor);   

        ResourceMaterial* material = new ResourceMaterial(materialUID, FileSystem::GetFileNameWithoutExtension(path));

        material->LoadMaterialData(mat);

        delete[] buffer;

        //App->GetLibraryModule()->AddResource(savePath, finalMeshUID);

        return material;
    }