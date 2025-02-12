#include "MaterialImporter.h"

#include "Application.h"
#include "FileSystem.h"
#include "LibraryModule.h"
#include "Material.h"
#include "TextureImporter.h"
#include <FileSystem>

UID MaterialImporter::ImportMaterial(const tinygltf::Model &model, int materialIndex)
{

    const tinygltf::Material &gltfMaterial = model.materials[materialIndex];

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

            UID diffuseUID = TextureImporter::Import(model.images[model.textures[texIndex].source].uri.c_str());

            if (diffuseUID == 0)
            {
              return 0;
            }

            material.SetDiffuseTexture(diffuseUID);
        }

        if (specGloss.Has("glossinessFactor"))
        {
            material.SetGlossinessFactor(static_cast<float>(specGloss.Get("glossinessFactor").Get<double>()));
        }

        // Specular Factor
        if (specGloss.Has("specularFactor"))
        {
            const std::vector<tinygltf::Value> &specArray = diffuseValue.Get<tinygltf::Value::Array>();

            float3 specular                               = {
                static_cast<float>(specArray[0].Get<double>()), static_cast<float>(specArray[1].Get<double>()),
                static_cast<float>(specArray[2].Get<double>())
            };

            material.SetSpecularFactor(specular);
        }

        // Specular-Glossiness Texture
        if (specGloss.Has("specularGlossinessTexture"))
        {
            const tinygltf::Value &specTex = specGloss.Get("specularGlossinessTexture");
            int texIndex                   = specTex.Get("index").Get<int>();


            UID specularGlossinessUID = TextureImporter::Import(model.images[model.textures[texIndex].source].uri.c_str());

            if (specularGlossinessUID == 0)
            {
                return 0;
            }

            material.SetSpecularGlossinessTexture(specularGlossinessUID);
        }
    }

    // Normal Map
    if (gltfMaterial.normalTexture.index >= 0)
    {
        int texIndex = gltfMaterial.normalTexture.index;

        UID normalUID = TextureImporter::Import(model.images[model.textures[texIndex].source].uri.c_str());

        if (normalUID == 0)
        {
            return 0;
        }

        material.SetNormalTexture(normalUID);
    }


    if (gltfMaterial.occlusionTexture.index >= 0)
    {
        int texIndex = gltfMaterial.occlusionTexture.index;

        material.SetOcclusionStrength(static_cast<float>(gltfMaterial.occlusionTexture.strength));

        UID occlusionUID = TextureImporter::Import(model.images[model.textures[texIndex].source].uri.c_str());

        if (occlusionUID == 0)
        {
            return 0;
        }

        material.SetOcclusionTexture(occlusionUID);
    }

    return true;
}