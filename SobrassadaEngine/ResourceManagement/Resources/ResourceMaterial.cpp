#include "ResourceMaterial.h"

#include "Application.h"
#include "EditorUIModule.h"
#include "FileSystem.h"
#include "FileSystem/Material.h"
#include "LibraryModule.h"
#include "ProjectModule.h"
#include "ResourceTexture.h"
#include "TextureImporter.h"

#include "glew.h"
#include "imgui.h"
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

ResourceMaterial::ResourceMaterial(UID uid, const std::string& name, const rapidjson::Value& importOptions)
    : Resource(uid, name, ResourceType::Material)
{
    if (importOptions.HasMember("defaultTextureUID") && importOptions["defaultTextureUID"].IsUint64())
        defaultTextureUID = importOptions["defaultTextureUID"].GetUint64();

    else defaultTextureUID = INVALID_UID;

    if (importOptions.HasMember("isTransparent") && importOptions["isTransparent"].IsBool())
        isTransparent = importOptions["isTransparent"].GetBool();
    else isTransparent = false;

    if (importOptions.HasMember("isAlphaDiscard") && importOptions["isAlphaDiscard"].IsBool())
        isAlpha = importOptions["isAlphaDiscard"].GetBool();
    else isAlpha = false;

    if (importOptions.HasMember("isDoubleSided") && importOptions["isDoubleSided"].IsBool())
        doubleSided = importOptions["isDoubleSided"].GetBool();
    else doubleSided = false;
}

ResourceMaterial::~ResourceMaterial()
{
    FreeMaterials();
}

void ResourceMaterial::OnEditorUpdate()
{
    bool updated  = false;

    updated      |= ImGui::Checkbox("Double Sided", &doubleSided);
    if (diffuseTexture.textureID != 0)
    {
        ImGui::Text("Diffuse Texture");
        ImGui::Image((ImTextureID)(intptr_t)diffuseTexture.textureID, ImVec2(256, 256));
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Texture Dimensions: %d, %d", diffuseTexture.width, diffuseTexture.height);
        }

        ImGui::SameLine();

        // TODO: commented all select buttons until save data to meta is implemented
        /*if (ImGui::Button("Select Diffuse Texture"))
        {
            ImGui::OpenPopup(CONSTANT_DIFFUSE_TEXTURE_SELECT_DIALOG_ID);
        }

        if (ImGui::IsPopupOpen(CONSTANT_DIFFUSE_TEXTURE_SELECT_DIALOG_ID))
        {
            UID handle = ChangeTexture(
                App->GetEditorUIModule()->RenderResourceSelectDialog<UID>(
                    CONSTANT_DIFFUSE_TEXTURE_SELECT_DIALOG_ID, App->GetLibraryModule()->GetTextureMap(), INVALID_UID
                ),
                diffuseTexture, material.diffuseTex
            );

            if (handle != NULL)
            {
                material.diffuseTex = handle;
                updated             = true;
            }
        }*/
    }

    updated |= ImGui::SliderFloat3("Diffuse Color", &material.diffColor.x, 0.0f, 1.0f);

    if (specularTexture.textureID != 0)
    {
        ImGui::Text("Specular Texture");
        ImGui::Image((ImTextureID)(intptr_t)specularTexture.textureID, ImVec2(256, 256));
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Texture Dimensions: %d, %d", specularTexture.width, specularTexture.height);
        }

        // TODO: commented all select buttons until save data to meta is implemented
        /*if (ImGui::Button("Select Specular Texture"))
        {
            ImGui::OpenPopup(CONSTANT_TEXTURE_SELECT_DIALOG_ID);
        }

        if (ImGui::IsPopupOpen(CONSTANT_TEXTURE_SELECT_DIALOG_ID))
        {
            UID handle = ChangeTexture(
                App->GetEditorUIModule()->RenderResourceSelectDialog<UID>(
                    CONSTANT_TEXTURE_SELECT_DIALOG_ID, App->GetLibraryModule()->GetTextureMap(), INVALID_UID
                ),
                specularTexture, material.specularTex
            );

            if (handle != NULL)
            {
                material.specularTex = handle;
                updated              = true;
            }
        }*/

        updated |= ImGui::SliderFloat3("Specular Color", &material.specColor.x, 0.0f, 1.0f);
        if (!material.shininessInAlpha) updated |= ImGui::SliderFloat("Shininess", &material.shininess, 0.0f, 500.0f);
    }

    else
    {
        if (metallicTexture.textureID != 0)
        {
            ImGui::Text("Metallic Roughness Texture");
            ImGui::Image((ImTextureID)(intptr_t)metallicTexture.textureID, ImVec2(256, 256));
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Texture Dimensions: %d, %d", metallicTexture.width, metallicTexture.height);
            }
        }

        // TODO: commented all select buttons until save data to meta is implemented
        /*ImGui::SameLine();
        if (ImGui::Button("Select Metallic Texture"))
        {
            ImGui::OpenPopup(CONSTANT_METALLIC_TEXTURE_SELECT_DIALOG_ID);
        }

        if (ImGui::IsPopupOpen(CONSTANT_METALLIC_TEXTURE_SELECT_DIALOG_ID))
        {
            UID handle = ChangeTexture(
                App->GetEditorUIModule()->RenderResourceSelectDialog<UID>(
                    CONSTANT_METALLIC_TEXTURE_SELECT_DIALOG_ID, App->GetLibraryModule()->GetTextureMap(), INVALID_UID
                ),
                metallicTexture, material.metallicTex
            );

            if (handle != NULL)
            {
                material.metallicTex = handle;
                updated              = true;
            }
        }*/

        updated |= ImGui::SliderFloat("Metallic Factor", &material.metallicFactor, 0.0f, 1.0f);
        updated |= ImGui::SliderFloat("Roughness Factor", &material.roughnessFactor, 0.0f, 1.0f);
    }

    if (normalTexture.textureID != 0)
    {
        ImGui::Text("Normal Texture");
        ImGui::Image((ImTextureID)(intptr_t)normalTexture.textureID, ImVec2(256, 256));
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Texture Dimensions: %d, %d", normalTexture.width, normalTexture.height);
        }
        // TODO: commented all select buttons until save data to meta is implemented
        /*ImGui::SameLine();
        if (ImGui::Button("Select Normal Texture"))
        {
            ImGui::OpenPopup(CONSTANT_TEXTURE_SELECT_DIALOG_ID);
        }

        if (ImGui::IsPopupOpen(CONSTANT_TEXTURE_SELECT_DIALOG_ID))
        {
            UID handle = ChangeTexture(
                App->GetEditorUIModule()->RenderResourceSelectDialog<UID>(
                    CONSTANT_TEXTURE_SELECT_DIALOG_ID, App->GetLibraryModule()->GetTextureMap(), INVALID_UID
                ),
                normalTexture, material.normalTex
            );

            if (handle != NULL)
            {
                material.normalTex = handle;
                updated              = true;
            }
        }*/
    }

    if (updated) SaveToMeta();
}

void ResourceMaterial::SaveToMeta()
{
    std::string path               = App->GetLibraryModule()->GetResourcePath(uid);
    const std::string& projectPath = App->GetProjectModule()->GetLoadedProjectPath();
    rapidjson::Document doc;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(projectPath + METADATA_PATH))
    {
        if (FileSystem::GetFileExtension(entry.path().string()) == META_EXTENSION)
        {
            std::string filePath = entry.path().string();
            if (!FileSystem::LoadJSON(filePath.c_str(), doc)) continue;

            UID assetUID = doc["UID"].GetUint64();

            if (uid == assetUID)
            {
                rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

                // Read
                UID shaderUID                                 = 0;
                bool useOcclusion                             = false;
                if (doc.HasMember("importOptions") && doc["importOptions"].IsObject())
                {
                    const auto& opts = doc["importOptions"];
                    if (opts.HasMember("shader") && opts["shader"].IsUint64()) shaderUID = opts["shader"].GetUint64();
                    if (opts.HasMember("useOcclusion") && opts["useOcclusion"].IsBool())
                        useOcclusion = opts["useOcclusion"].GetBool();
                }

                rapidjson::Value importOptions(rapidjson::kObjectType);
                importOptions.AddMember("shader", rapidjson::Value().SetUint64(shaderUID), allocator);
                importOptions.AddMember("useOcclusion", useOcclusion, allocator);
                importOptions.AddMember(
                    "defaultTextureUID", rapidjson::Value().SetUint64(defaultTextureUID), allocator
                );
                importOptions.AddMember("isTransparent", isTransparent, allocator);
                importOptions.AddMember("isAlphaDiscard", isAlpha, allocator);
                importOptions.AddMember("isDoubleSided", doubleSided, allocator);

                if (doc.HasMember("importOptions")) doc["importOptions"] = importOptions;
                else doc.AddMember("importOptions", importOptions, allocator);

                rapidjson::StringBuffer buffer;
                rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
                doc.Accept(writer);

                FileSystem::Save(filePath.c_str(), buffer.GetString(), (unsigned int)buffer.GetSize(), false);
                return;
            }
        }
    }
}

UID ResourceMaterial::ChangeTexture(UID newTexture, TextureInfo& textureToChange, UID textureGPU)
{
    if (newTexture == INVALID_UID) return NULL;

    ResourceTexture* texture = TextureImporter::LoadTexture(newTexture);
    if (texture != nullptr)
    {
        glMakeTextureHandleNonResidentARB(textureGPU);
        glDeleteTextures(1, &textureToChange.textureID);

        const UID handle = glGetTextureHandleARB(texture->GetTextureID());
        glMakeTextureHandleResidentARB(handle);

        textureToChange.textureID = texture->GetTextureID();
        textureToChange.width     = texture->GetTextureWidth();
        textureToChange.height    = texture->GetTextureHeight();

        return handle;
    }
    delete texture;
    return NULL;
}

void ResourceMaterial::ChangeFallBackTexture()
{
    ChangeTexture(FALLBACK_TEXTURE_UID, diffuseTexture, material.diffuseTex);
}

void ResourceMaterial::LoadMaterialData(Material mat)
{
    material.diffColor           = mat.GetDiffuseFactor();
    material.specColor           = mat.GetSpecularFactor();
    material.shininess           = mat.GetGlossinessFactor();
    material.metallicFactor      = mat.GetMetallicFactor();
    material.roughnessFactor     = mat.GetRoughnessFactor();
    material.shininessInAlpha    = false;
    isTransparent                = mat.IsTransparent();
    isAlpha                      = mat.IsAlphaDiscard();
    doubleSided                  = mat.IsDoubleSided();

    ResourceTexture* diffTexture = TextureImporter::LoadTexture(mat.GetDiffuseTexture());
    if (diffTexture != nullptr)
    {
        diffuseTexture.textureID = diffTexture->GetTextureID();

        material.diffuseTex      = glGetTextureHandleARB(diffTexture->GetTextureID());
        glMakeTextureHandleResidentARB(material.diffuseTex);

        diffuseTexture.width  = diffTexture->GetTextureWidth();
        diffuseTexture.height = diffTexture->GetTextureHeight();
    }
    else
    {
        ResourceTexture* fallbackTexture = TextureImporter::LoadTexture(FALLBACK_TEXTURE_UID);
        diffuseTexture.textureID         = fallbackTexture->GetTextureID();

        material.diffuseTex              = glGetTextureHandleARB(fallbackTexture->GetTextureID());
        glMakeTextureHandleResidentARB(material.diffuseTex);

        diffuseTexture.width  = fallbackTexture->GetTextureWidth();
        diffuseTexture.height = fallbackTexture->GetTextureHeight();

        delete fallbackTexture;
    }

    if (diffuseTexture.textureID == 0)
    {
        ResourceTexture* diffTexture = TextureImporter::LoadTexture(FALLBACK_TEXTURE_UID);
        if (diffTexture != nullptr)
        {
            diffuseTexture.textureID = diffTexture->GetTextureID();

            material.diffuseTex      = glGetTextureHandleARB(diffTexture->GetTextureID());
            glMakeTextureHandleResidentARB(material.diffuseTex);

            diffuseTexture.width  = diffTexture->GetTextureWidth();
            diffuseTexture.height = diffTexture->GetTextureHeight();
        }
    }

    ResourceTexture* metallicRoughnessTexture = TextureImporter::LoadTexture(mat.GetMetallicRoughnessTexture());
    if (metallicRoughnessTexture != nullptr)
    {
        metallicTexture.textureID = metallicRoughnessTexture->GetTextureID();

        material.metallicTex      = glGetTextureHandleARB(metallicRoughnessTexture->GetTextureID());
        glMakeTextureHandleResidentARB(material.metallicTex);

        metallicTexture.width  = metallicRoughnessTexture->GetTextureWidth();
        metallicTexture.height = metallicRoughnessTexture->GetTextureHeight();
        material.hasMetallic   = 1;
    }

    if (metallicTexture.textureID == 0)
    {
        ResourceTexture* specTexture = TextureImporter::LoadTexture(mat.GetSpecularGlossinessTexture());
        if (specTexture != nullptr)
        {
            specularTexture.textureID = specTexture->GetTextureID();

            material.specularTex      = glGetTextureHandleARB(specTexture->GetTextureID());
            glMakeTextureHandleResidentARB(material.specularTex);

            specularTexture.width     = specTexture->GetTextureWidth();
            specularTexture.height    = specTexture->GetTextureHeight();

            material.shininessInAlpha = true;
            material.hasSpecular      = 1;
        }

        delete specTexture;
    }

    ResourceTexture* normTexture = TextureImporter::LoadTexture(mat.GetNormalTexture());
    if (normTexture != nullptr)
    {
        // GLOG("%s has normal", normTexture->GetName().c_str());
        normalTexture.textureID = normTexture->GetTextureID();

        material.normalTex      = glGetTextureHandleARB(normTexture->GetTextureID());
        glMakeTextureHandleResidentARB(material.normalTex);

        normalTexture.width  = normTexture->GetTextureWidth();
        normalTexture.height = normTexture->GetTextureHeight();
    }

    delete diffTexture;
    delete metallicRoughnessTexture;
    delete normTexture;
}

void ResourceMaterial::FreeMaterials() const
{
    if (diffuseTexture.textureID != 0)
    {
        glMakeTextureHandleNonResidentARB(material.diffuseTex);
        glDeleteTextures(1, &diffuseTexture.textureID);
    }

    if (specularTexture.textureID != 0)
    {
        glMakeTextureHandleNonResidentARB(material.specularTex);
        glDeleteTextures(1, &specularTexture.textureID);
    }

    if (metallicTexture.textureID != 0)
    {
        glMakeTextureHandleNonResidentARB(material.metallicTex);
        glDeleteTextures(1, &metallicTexture.textureID);
    }

    if (normalTexture.textureID != 0)
    {
        glMakeTextureHandleNonResidentARB(material.normalTex);
        glDeleteTextures(1, &normalTexture.textureID);
    }
}