#include "MaterialImporter.h"

#include "Application.h"
#include "FileSystem.h"
#include "LibraryModule.h"
#include "Material.h"
#include "MetaMaterial.h"
#include "ProjectModule.h"
#include "ResourceManagement/Resources/ResourceMaterial.h"
#include "TextureImporter.h"

#include <FileSystem>
#include <tiny_gltf.h>

UID MaterialImporter::ImportMaterial(
    const tinygltf::Model& model, int materialIndex, const char* sourceFilePath, const std::string& targetFilePath,
    UID sourceUID, UID defaultTextureUID
)
{
    // If it has no materials exit
    if (materialIndex == -1) return INVALID_UID;

    std::string path                       = FileSystem::GetFilePath(sourceFilePath);
    bool useOcclusion                      = false;
    const tinygltf::Material& gltfMaterial = model.materials[materialIndex];
    std::string materialName               = gltfMaterial.name;
    const bool isTransparent               = gltfMaterial.alphaMode == "BLEND";
    const bool isAlphaDiscard              = gltfMaterial.alphaMode == "MASK";
    const bool isDoubleSided               = gltfMaterial.doubleSided;

    int sizeofStrings                      = 0;
    auto it                                = gltfMaterial.extensions.find("KHR_materials_pbrSpecularGlossiness");
    // ADD OLD LOADING

    Material material;
    // KHR_materials_pbrSpecularGlossiness extension
    if (it != gltfMaterial.extensions.end())
    {
        const tinygltf::Value& specGloss    = it->second;

        // Diffuse Factor
        const tinygltf::Value& diffuseValue = specGloss.Get("diffuseFactor");
        if (diffuseValue.IsArray() && diffuseValue.ArrayLen() == 4)
        {
            const std::vector<tinygltf::Value>& diffuseArray = diffuseValue.Get<tinygltf::Value::Array>();

            float4 diffuseFactor(
                static_cast<float>(diffuseArray[0].Get<double>()), static_cast<float>(diffuseArray[1].Get<double>()),
                static_cast<float>(diffuseArray[2].Get<double>()), static_cast<float>(diffuseArray[3].Get<double>())
            );

            material.SetDiffuseFactor(diffuseFactor);
        }

        // Diffuse Texture
        if (specGloss.Has("diffuseTexture"))
        {
            const tinygltf::Value& diffuseTex = specGloss.Get("diffuseTexture");
            int texIndex                      = diffuseTex.Get("index").Get<int>();

            UID diffuseUID =
                HandleTextureImport(path + model.images[model.textures[texIndex].source].uri, targetFilePath);

            if (diffuseUID != INVALID_UID)
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
            const std::vector<tinygltf::Value>& specArray = diffuseValue.Get<tinygltf::Value::Array>();

            if (specArray.size() > 3)
            {
                float3 specular = {
                    static_cast<float>(specArray[0].Get<double>()), static_cast<float>(specArray[1].Get<double>()),
                    static_cast<float>(specArray[2].Get<double>())
                };

                material.SetSpecularFactor(specular);
            }
        }

        // Specular-Glossiness Texture
        if (specGloss.Has("specularGlossinessTexture"))
        {
            const tinygltf::Value& specTex = specGloss.Get("specularGlossinessTexture");
            int texIndex                   = specTex.Get("index").Get<int>();

            UID specularGlossinessUID =
                HandleTextureImport(path + model.images[model.textures[texIndex].source].uri, targetFilePath);

            if (specularGlossinessUID != INVALID_UID)
            {
                material.SetSpecularGlossinessTexture(specularGlossinessUID);
            }
        }
    }
    // pbrMetallicRoughness extension
    else if (gltfMaterial.pbrMetallicRoughness.baseColorFactor.size() == 4)
    {
        const auto& pbr = gltfMaterial.pbrMetallicRoughness;

        // Base Color Factor
        float4 baseColorFactor(
            static_cast<float>(pbr.baseColorFactor[0]), static_cast<float>(pbr.baseColorFactor[1]),
            static_cast<float>(pbr.baseColorFactor[2]), static_cast<float>(pbr.baseColorFactor[3])
        );
        material.SetDiffuseFactor(baseColorFactor);

        // Base Color Texture
        if (pbr.baseColorTexture.index >= 0)
        {
            int texIndex = pbr.baseColorTexture.index;
            UID diffuseUID =
                HandleTextureImport(path + model.images[model.textures[texIndex].source].uri, targetFilePath);

            if (diffuseUID != INVALID_UID)
            {
                material.SetDiffuseTexture(diffuseUID);
            }
        }

        if (pbr.metallicRoughnessTexture.index >= 0)
        {
            int texIndex = pbr.metallicRoughnessTexture.index;
            UID metallicRoughnessTextureUID =
                HandleTextureImport(path + model.images[model.textures[texIndex].source].uri, targetFilePath);
            if (metallicRoughnessTextureUID != INVALID_UID)
            {
                material.SetMetallicRoughnessTexture(metallicRoughnessTextureUID);
            }
        }

        // Metallic and Roughness Factors
        material.SetMetallicFactor(static_cast<float>(pbr.metallicFactor));
        material.SetRoughnessFactor(static_cast<float>(pbr.roughnessFactor));
    }

    // Normal Map
    if (gltfMaterial.normalTexture.index >= 0)
    {
        int texIndex  = gltfMaterial.normalTexture.index;

        UID normalUID = HandleTextureImport(path + model.images[model.textures[texIndex].source].uri, targetFilePath);
        if (normalUID != INVALID_UID)
        {
            material.SetNormalTexture(normalUID);
        }
    }

    // TODO: SOLVE OCCLUSION NOT LOADING
    if (gltfMaterial.occlusionTexture.index >= 0)
    {
        int texIndex = gltfMaterial.occlusionTexture.index;
        useOcclusion = true;
        material.SetOcclusionStrength(static_cast<float>(gltfMaterial.occlusionTexture.strength));

        UID occlusionUID =
            HandleTextureImport(path + model.images[model.textures[texIndex].source].uri, targetFilePath);
        if (occlusionUID != INVALID_UID)
        {
            material.SetOcclusionTexture(occlusionUID);
        }
    }

    material.SetAlphaDiscard(isAlphaDiscard);
    material.SetTransparent(isTransparent);
    material.SetDoubleSide(isDoubleSided);

    unsigned int size = sizeof(Material);
    char* fileBuffer  = new char[size];
    memcpy(fileBuffer, &material, sizeof(Material));

    UID finalMaterialUID;
    if (sourceUID == INVALID_UID)
    {
        UID materialUID            = GenerateUID();
        materialUID                = App->GetLibraryModule()->AssignFiletypeUID(materialUID, FileType::Material);

        UID prefix                 = materialUID / UID_PREFIX_DIVISOR;
        const std::string savePath = App->GetProjectModule()->GetLoadedProjectPath() + METADATA_PATH +
                                     std::to_string(prefix) + FILENAME_SEPARATOR + materialName + META_EXTENSION;
        finalMaterialUID = App->GetLibraryModule()->GetUIDFromMetaFile(savePath);
        if (finalMaterialUID == INVALID_UID) finalMaterialUID = materialUID;

        // replace "" with shader used (example)
        UID tmpName               = GenerateUID();
        std::string tmpNameString = std::to_string(tmpName);

        if (materialName.empty()) materialName = "MaterialType_" + std::to_string(tmpName);

        std::string assetPath = ASSETS_PATH + FileSystem::GetFileNameWithExtension(sourceFilePath);
        MetaMaterial meta(
            finalMaterialUID, assetPath, tmpNameString, useOcclusion, defaultTextureUID, isTransparent, isDoubleSided,
            isAlphaDiscard
        );
        meta.Save(materialName, assetPath);
    }
    else finalMaterialUID = sourceUID;

    material.SetMaterialUID(finalMaterialUID);

    std::string saveFilePath  = targetFilePath + MATERIALS_PATH + std::to_string(finalMaterialUID) + MATERIAL_EXTENSION;
    unsigned int bytesWritten = (unsigned int)FileSystem::Save(saveFilePath.c_str(), fileBuffer, size, true);

    delete[] fileBuffer;

    if (bytesWritten == 0)
    {
        GLOG("Failed to save material: %s", materialName.c_str());
        return 0;
    }

    App->GetLibraryModule()->AddMaterial(finalMaterialUID, materialName);
    App->GetLibraryModule()->AddName(materialName, finalMaterialUID);
    App->GetLibraryModule()->AddResource(saveFilePath, finalMaterialUID);

    // GLOG("%s saved as material", materialName.c_str());

    return finalMaterialUID;
}

UID MaterialImporter::HandleTextureImport(const std::string& filePath, const std::string& targetFilePath)
{
    UID textureUID = App->GetLibraryModule()->GetTextureUID(FileSystem::GetFileNameWithoutExtension(filePath));
    textureUID     = TextureImporter::Import(filePath.c_str(), targetFilePath, textureUID, true);
    return textureUID;
}

ResourceMaterial* MaterialImporter::LoadMaterial(UID materialUID)
{
    char* buffer          = nullptr;

    std::string path      = App->GetLibraryModule()->GetResourcePath(materialUID);
    std::string name      = App->GetLibraryModule()->GetResourceName(materialUID);

    unsigned int fileSize = FileSystem::Load(path.c_str(), &buffer);

    if (fileSize == 0 || buffer == nullptr)
    {
        GLOG("Failed to load the .mat file: ");
        return nullptr;
    }

    char* cursor = buffer;

    rapidjson::Document doc;
    rapidjson::Value importOptions;
    App->GetLibraryModule()->GetImportOptions(materialUID, doc, importOptions);

    // Create Mesh
    Material mat               = *reinterpret_cast<Material*>(cursor);

    ResourceMaterial* material = new ResourceMaterial(materialUID, name);

    material->LoadMaterialData(mat, importOptions);

    delete[] buffer;

    return material;
}