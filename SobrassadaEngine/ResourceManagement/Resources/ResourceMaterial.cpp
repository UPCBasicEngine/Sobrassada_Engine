#include "ResourceMaterial.h"

#include "Application.h"
#include "DirectXTex/DirectXTex.h"
#include "TextureImporter.h"
#include "imgui.h"
#include "LibraryModule.h"

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

void ResourceMaterial::LoadMaterialData(Material mat)
{
    material.diffColor    = mat.GetDiffuseFactor();
    material.specColor    = mat.GetSpecularFactor();
    material.shininessInAlpha = false;
    material.shininess    = mat.GetGlossinessFactor();

    ResourceTexture* diffTexture = TextureImporter::LoadTexture(mat.GetDiffuseTexture());
    if (diffTexture != nullptr)
    {
        diffuseTexture.textureID     = diffTexture->GetTextureID();
        hasDiffuseTexture            = true;
    }

    ResourceTexture* specTexture = TextureImporter::LoadTexture(mat.GetSpecularGlossinessTexture());
    if (specTexture != nullptr)
    {
        specularTexture.textureID     = specTexture->GetTextureID();
        hasSpecularTexture            = true;
    }
    

    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(MaterialGPU), &material, GL_DYNAMIC_DRAW);
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
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(MaterialGPU), &material);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}