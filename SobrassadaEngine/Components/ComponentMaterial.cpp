#include "ComponentMaterial.h"
#include "imgui.h"
#include "TextureModuleTest.h"
#include "Application.h"
#include <unordered_set>
#include "Math/float2.h"
#include "DirectXTex/DirectXTex.h"
#include <glew.h>

void ComponentMaterial::OnEditorUpdate() 
{ 
    if (ImGui::CollapsingHeader("Material"))
    {
        if (hasDiffuseTexture)
        {   
            ImGui::Text("Diffuse Texture");
            ImGui::Image((ImTextureID)(intptr_t)diffuseTexture.textureID, ImVec2(256, 256));
            if(ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Texture Dimensions: %d, %d", diffuseTexture.width, diffuseTexture.height);
            }
        }
        ImGui::SliderFloat3("Diffuse Color", &diffuseColor.x, 0.0f, 1.0f);

        if (hasSpecularTexture)
        {
            ImGui::Text("Specular Texture");
            ImGui::Image((ImTextureID)(intptr_t)specularTexture.textureID, ImVec2(256, 256));
            if(ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Texture Dimensions: %d, %d", specularTexture.width, specularTexture.height);
            }
        }
        ImGui::SliderFloat3("Specular Color", &specularColor.x, 0.0f, 1.0f);
        if (!hasShininessInAlpha) ImGui::SliderFloat("Shininess", &shininess, 0.0f, 500.0f);
    }
}

TextureInfo ComponentMaterial::GetTexture(const tinygltf::Model sourceModel, int textureIndex, const char* modelPath) {
	const tinygltf::Texture &texture = sourceModel.textures[textureIndex];
    const tinygltf::Image &image     = sourceModel.images[texture.source];
    std::unordered_set<int> loadedIndices;
    TextureInfo info;
                    
    if (loadedIndices.find(texture.source) == loadedIndices.end())
    {
        std::string filePath     = std::string(modelPath);
        char usedSeparator       = '\\';

        int fileLocationPosition = (int)filePath.find_last_of(usedSeparator);
        if (fileLocationPosition == -1)
        {
            usedSeparator        = '/';
            fileLocationPosition = filePath.find_last_of(usedSeparator);
        }
                        
        if (fileLocationPosition == -1) return info;

        std::string fileLocation      = filePath.substr(0, fileLocationPosition) + usedSeparator;
        std::string texturePathString = fileLocation.append(image.uri);

        std::wstring wideUri       = std::wstring(texturePathString.begin(), texturePathString.end());
        const wchar_t *texturePath = wideUri.c_str();

        DirectX::TexMetadata textureMetadata;
        unsigned int textureId = App->GetTextureModuleTest()->LoadTexture(texturePath, textureMetadata);
        if (textureId)
        {   
            info.textureID = textureId;
            info.width = textureMetadata.width;
            info.height = textureMetadata.height;
            loadedIndices.insert(texture.source);

			return info;
        }
    }
}

void ComponentMaterial::LoadMaterial(const tinygltf::Material &srcMaterial, const tinygltf::Model &sourceModel, const char *modelPath)
{
    name = srcMaterial.name;
    unsigned int textureId = 0;
        
	auto it = srcMaterial.extensions.find("KHR_materials_pbrSpecularGlossiness");
    if (it != srcMaterial.extensions.end())
    {
        const tinygltf::Value &ext = it->second;

        if (ext.Has("diffuseFactor"))
        {
            const tinygltf::Value &diffuseValue = ext.Get("diffuseFactor");
            if (diffuseValue.IsArray() && diffuseValue.ArrayLen() == 4)
            {
                diffuseColor =
                {
                        static_cast<float>(diffuseValue.Get(0).Get<double>()),
                        static_cast<float>(diffuseValue.Get(1).Get<double>()),
                        static_cast<float>(diffuseValue.Get(2).Get<double>()),
                };
            }
        }

        if (ext.Has("specularFactor"))
        {
            const tinygltf::Value &specularValue = ext.Get("specularFactor");
            if (specularValue.IsArray() && specularValue.ArrayLen() == 3)
            {
                specularColor =
                {
                    static_cast<float>(specularValue.Get(0).Get<double>()),
                    static_cast<float>(specularValue.Get(1).Get<double>()),
                    static_cast<float>(specularValue.Get(2).Get<double>())
                };
            }
        }

        if (ext.Has("glossinessFactor"))
        {
            shininess = ext.Get("glossinessFactor").Get<double>();
        }

        if (ext.Has("specularGlossinessTexture"))
        {
            int textureIndex = ext.Get("specularGlossinessTexture").Get("index").Get<int>();
                
            if (textureIndex >= 0)
            {
                hasSpecularTexture = true;

                specularTexture = GetTexture(sourceModel, textureIndex, modelPath);
                hasShininessInAlpha = true;
            }
        }

		if (ext.Has("diffuseTexture"))
        {
            int textureIndex = ext.Get("diffuseTexture").Get("index").Get<int>();
                
            if (textureIndex >= 0)
            {
				hasDiffuseTexture = true;

                diffuseTexture = GetTexture(sourceModel, textureIndex, modelPath);
            }
        }
    }

    else
    {  
        int textureIndex       = srcMaterial.pbrMetallicRoughness.baseColorTexture.index;
        if (textureIndex < 0)
        {
            diffuseColor = 
            {
                static_cast<float>(srcMaterial.pbrMetallicRoughness.baseColorFactor[0]),
                static_cast<float>(srcMaterial.pbrMetallicRoughness.baseColorFactor[1]),
                static_cast<float>(srcMaterial.pbrMetallicRoughness.baseColorFactor[2])
            };
        }
        else
        {
			hasDiffuseTexture = true;

            diffuseTexture = GetTexture(sourceModel, textureIndex, modelPath);
        }
    }
}

void ComponentMaterial::RenderMaterial(int program) {
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

    Material material;
    material.diffFactor = float4(diffuseColor, 1.0f);
    material.specFactor = float4(specularColor, 1.0f);
    material.shininess  = shininess;
    material.shininessInAlpha = hasShininessInAlpha;

    unsigned int ubo          = 0;
    glGenBuffers(1, &ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(material), &material, GL_STATIC_DRAW);
    unsigned int blockIdx = glGetUniformBlockIndex(program, "Material");


    glUniformBlockBinding(program, blockIdx, 2);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, ubo);

    if (!hasShininessInAlpha)
    {
        glUniform1f(glGetUniformLocation(program, "shininess"), shininess);
    }
    glUniform3fv(glGetUniformLocation(program, "diffFactor"), 1, &diffuseColor[0]);
    glUniform3fv(glGetUniformLocation(program, "specFactor"), 1, &specularColor[0]);

    glDeleteBuffers(1, &ubo);
}