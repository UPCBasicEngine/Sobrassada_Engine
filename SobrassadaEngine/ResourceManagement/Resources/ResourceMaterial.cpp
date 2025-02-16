#include "ResourceMaterial.h"

#include "Application.h"
#include "DirectXTex/DirectXTex.h"
#include "TextureImporter.h"
#include "imgui.h"

#include <glew.h>
#include <unordered_set>

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>

ResourceMaterial::ResourceMaterial(UID uid, const std::string& name) : Resource(uid, name, ResourceType::Material)
{
}

ResourceMaterial::~ResourceMaterial()
{
    FreeMaterials();
}

void ResourceMaterial::OnEditorUpdate()
{
    bool updated = false;

    if (hasDiffuseTexture)
    {
        ImGui::Text("Diffuse Texture");
        ImGui::Image((ImTextureID)(intptr_t)diffuseTexture.textureID, ImVec2(256, 256));
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Texture Dimensions: %d, %d", diffuseTexture.width, diffuseTexture.height);
        }
    }

    updated |= ImGui::SliderFloat3("Diffuse Color", &material.diffColor.x, 0.0f, 1.0f);

    if (hasSpecularTexture)
    {
        ImGui::Text("Specular Texture");
        ImGui::Image((ImTextureID)(intptr_t)specularTexture.textureID, ImVec2(256, 256));
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Texture Dimensions: %d, %d", specularTexture.width, specularTexture.height);
        }
    }

    updated |= ImGui::SliderFloat3("Specular Color", &material.specColor.x, 0.0f, 1.0f);

    if (!material.shininessInAlpha) updated |= ImGui::SliderFloat("Shininess", &material.shininess, 0.0f, 500.0f);

    if (updated) UpdateUBO();
}

TextureInfo ResourceMaterial::GetTexture(const tinygltf::Model sourceModel, int textureIndex, const char* modelPath)
{
    const tinygltf::Texture& texture = sourceModel.textures[textureIndex];
    const tinygltf::Image& image     = sourceModel.images[texture.source];
    std::unordered_set<int> loadedIndices;
    TextureInfo info;

    if (loadedIndices.find(texture.source) == loadedIndices.end())
    {
        std::string filePath     = std::string(modelPath);
        char usedSeparator       = '\\';

        int fileLocationPosition = static_cast<int>(filePath.find_last_of(usedSeparator));
        if (fileLocationPosition == -1)
        {
            usedSeparator        = '/';
            fileLocationPosition = static_cast<int>(filePath.find_last_of(usedSeparator));
        }

        if (fileLocationPosition == -1) return info;

        std::string fileLocation      = filePath.substr(0, fileLocationPosition) + usedSeparator;
        std::string texturePathString = fileLocation.append(image.uri);

        std::wstring wideUri          = std::wstring(texturePathString.begin(), texturePathString.end());
        const wchar_t* texturePath    = wideUri.c_str();

        DirectX::TexMetadata textureMetadata;
        unsigned int textureId = 0; // TextureImporter::LoadTexture(texturePath, textureMetadata);
        /*
        if (textureid)
        {
            info.textureid = textureid;
            info.width     = static_cast<int>(texturemetadata.width);
            info.height    = static_cast<int>(texturemetadata.height);
            loadedindices.insert(texture.source);

            return info;
        }
        */
        return info; //this is wrong
    }
}

void ResourceMaterial::LoadMaterial(
    const tinygltf::Material& srcMaterial, const tinygltf::Model& sourceModel, const char* modelPath
)
{
    name                   = srcMaterial.name;
    unsigned int textureId = 0;

    auto it                = srcMaterial.extensions.find("KHR_materials_pbrSpecularGlossiness");
    if (it != srcMaterial.extensions.end())
    {
        const tinygltf::Value& ext = it->second;

        if (ext.Has("diffuseFactor"))
        {
            const tinygltf::Value& diffuseValue = ext.Get("diffuseFactor");
            if (diffuseValue.IsArray() && diffuseValue.ArrayLen() == 4)
            {
                material.diffColor = {
                    static_cast<float>(diffuseValue.Get(0).Get<double>()),
                    static_cast<float>(diffuseValue.Get(1).Get<double>()),
                    static_cast<float>(diffuseValue.Get(2).Get<double>()),
                    static_cast<float>(diffuseValue.Get(3).Get<double>())
                };
            }
        }

        if (ext.Has("specularFactor"))
        {
            const tinygltf::Value& specularValue = ext.Get("specularFactor");
            if (specularValue.IsArray() && specularValue.ArrayLen() == 3)
            {
                material.specColor = {
                    static_cast<float>(specularValue.Get(0).Get<double>()),
                    static_cast<float>(specularValue.Get(1).Get<double>()),
                    static_cast<float>(specularValue.Get(2).Get<double>()), 1.0f
                };
            }
        }

        if (ext.Has("glossinessFactor"))
        {
            material.shininess = static_cast<float>(ext.Get("glossinessFactor").Get<double>());
        }

        if (ext.Has("specularGlossinessTexture"))
        {
            int textureIndex = ext.Get("specularGlossinessTexture").Get("index").Get<int>();

            if (textureIndex >= 0)
            {
                hasSpecularTexture        = true;

                specularTexture           = GetTexture(sourceModel, textureIndex, modelPath);
                material.shininessInAlpha = true;
            }
        }

        if (ext.Has("diffuseTexture"))
        {
            int textureIndex = ext.Get("diffuseTexture").Get("index").Get<int>();

            if (textureIndex >= 0)
            {
                hasDiffuseTexture = true;

                diffuseTexture    = GetTexture(sourceModel, textureIndex, modelPath);
            }
        }
    }

    else
    {
        int textureIndex = srcMaterial.pbrMetallicRoughness.baseColorTexture.index;
        if (textureIndex < 0)
        {
            material.diffColor = {
                static_cast<float>(srcMaterial.pbrMetallicRoughness.baseColorFactor[0]),
                static_cast<float>(srcMaterial.pbrMetallicRoughness.baseColorFactor[1]),
                static_cast<float>(srcMaterial.pbrMetallicRoughness.baseColorFactor[2]), 1.0f
            };
        }
        else
        {
            hasDiffuseTexture = true;

            diffuseTexture    = GetTexture(sourceModel, textureIndex, modelPath);
        }
    }

    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(Material), &material, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void ResourceMaterial::RenderMaterial(int program)
{
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);

    if (hasDiffuseTexture)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseTexture.textureID);
    }
    if (hasSpecularTexture)
    {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularTexture.textureID);
    }

    unsigned int blockIdx = glGetUniformBlockIndex(program, "Material");
    glUniformBlockBinding(program, blockIdx, 1);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void ResourceMaterial::FreeMaterials()
{
    glDeleteTextures(1, &diffuseTexture.textureID);
    glDeleteTextures(1, &specularTexture.textureID);
    glDeleteBuffers(1, &ubo);
}

void ResourceMaterial::UpdateUBO()
{
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Material), &material);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}