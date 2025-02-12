
#include "MaterialImporter.h"
#include "Globals.h"
#include "Material.h"
#include "TextureImporter.h"
#include <FileSystem>

bool MaterialImporter::ImportMaterial(const tinygltf::Model &model, int materialIndex)
{

    if (materialIndex < 0 || materialIndex >= model.materials.size())
    {
        GLOG("No material found.");
    }

    const tinygltf::Material &gltfMaterial = model.materials[materialIndex];

    Material material(std::move(gltfMaterial.name));

    auto it = gltfMaterial.extensions.find("KHR_materials_pbrSpecularGlossiness");

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
            int texIndex = diffuseTex.Get("index").Get<int>();

            material.SetDiffuseTexture(&model.images[model.textures[texIndex].source].uri);
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

        // Glossiness Factor

        // Specular-Glossiness Texture
        if (specGloss.Has("specularGlossinessTexture"))
        {
            // need a pointer to corresponding texture from textureImporter
            // int texIndex = specGlossExt.Get("specularGlossinessTexture").Get("index").Get<int>();
            // material.SetSpecularGlossinessTexture(&model.images[model.textures[texIndex].source].uri);
        }
    }

    // Normal Map
    if (gltfMaterial.normalTexture.index >= 0)
    {
        // need a pointer to corresponding texture from textureImporter
        // int texIndex = gltfMaterial.normalTexture.index;
        // material.SetNormalTexture(&model.images[model.textures[texIndex].source].uri);
    }

    // Occlusion Map
    if (gltfMaterial.occlusionTexture.index >= 0)
    {
        // need a pointer to corresponding texture from textureImporter
        // int texIndex = gltfMaterial.occlusionTexture.index;
        // material.SetOcclusionTexture(&model.images[model.textures[texIndex].source].uri);
        // material.SetOcclusionStrength(static_cast<float>(gltfMaterial.occlusionTexture.strength));
    }

    return true;
}