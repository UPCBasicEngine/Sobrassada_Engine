#include "ResourceMaterial.h"

#include "Application.h"
#include "EditorUIModule.h"
#include "FileSystem.h"
#include "FileSystem/Material.h"
#include "LibraryModule.h"
#include "ProjectModule.h"
#include "ResourceTexture.h"
#include "Scene.h"
#include "SceneModule.h"
#include "TextureImporter.h"
#include "WindConfig.h"

#include "glew.h"
#include "imgui.h"
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

ResourceMaterial::ResourceMaterial(UID uid, const std::string& name) : Resource(uid, name, ResourceType::Material)
{
}

ResourceMaterial::~ResourceMaterial()
{
    FreeMaterials();
}

void ResourceMaterial::OnEditorUpdate()
{
    bool updated  = false;

    updated      |= ImGui::Checkbox("Double Sided", &doubleSided);
    updated      |= ImGui::Checkbox("Apply wind", &applyWind);

    if (applyWind)
    {
        ImGui::Text("Material wind settings");
        updated |= ImGui::SliderFloat("UV 0 border", &vCoord0, 0, vCoord1 - 0.01f);
        updated |= ImGui::SliderFloat("UV 1 border", &vCoord1, vCoord0 + 0.01f, 1);
        updated |= ImGui::Checkbox("Use central pivot", &useCentralPivot);
        updated |= ImGui::Checkbox("Wind gravity", &useWindGravity);
        updated |= ImGui::SliderFloat("X amplitude", &windXAmplitude, 0, 2);
        updated |= ImGui::SliderFloat("Y amplitude", &windYAmplitude, 0, 2);
        updated |= ImGui::SliderFloat("Z amplitude", &windZAmplitude, 0, 2);
        updated |= ImGui::SliderFloat("X frequency", &windXFrequency, 0, 5);
        updated |= ImGui::SliderFloat("Y frequency", &windYFrequency, 0, 5);
        updated |= ImGui::SliderFloat("Z frequency", &windZFrequency, 0, 5);
        updated |= ImGui::SliderFloat("Time scale", &windTimeScale, 0, 10);

        ImGui::Text("Global wind settings");
        WindConfig* globalWindConfig = App->GetSceneModule()->GetScene()->GetWindsConfig();
        if (!globalWindConfig->GetApplyWindGlobally())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImVec4(1.f, 0.f, 0.f, 1.0f)));
            ImGui::Text("Global wind disabled, movement will not show!");
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImVec4(0.f, 1.f, 0.f, 1.0f)));
            ImGui::Text("Global wind active, movement will show");
            ImGui::PopStyleColor();
        }

        ImGui::Checkbox("Apply wind globally", &globalWindConfig->GetApplyWindGloballyRef());
        ImGui::SliderFloat(
            "Wind direction (Angle around y axis)", &globalWindConfig->GetWindDirectionRef(), 0.f, 360.f
        );
        ImGui::SliderFloat("Wind speed (m/s)", &globalWindConfig->GetWindSpeedRef(), 0.0f, 10.f);
        ImGui::SliderFloat("Gust frequency (1/s)", &globalWindConfig->GetGustFrequencyRef(), .3f, 10.f);
        ImGui::SliderFloat("Gust speed (m/s)", &globalWindConfig->GetGustSpeedRef(), 0.0f, 20.f);
    }

    if (ImGui::IsItemDeactivatedAfterEdit()) updated = true;

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
        if (ImGui::Button("Select Diffuse Texture"))
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
        }
    }

    ImGui::SliderFloat3("Diffuse Color", &material.diffColor.x, 0.0f, 1.0f);
    if (ImGui::IsItemDeactivatedAfterEdit()) updated = true;

    ImGui::SliderFloat("Alpha", &material.diffColor.w, 0.0f, 1.0f);
    if (ImGui::IsItemDeactivatedAfterEdit()) updated = true;

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

        ImGui::SliderFloat3("Specular Color", &material.specColor.x, 0.0f, 1.0f);
        if (ImGui::IsItemDeactivatedAfterEdit()) updated = true;

        if (!material.shininessInAlpha)
        {
            ImGui::SliderFloat("Shininess", &material.shininess, 0.0f, 500.0f);
            if (ImGui::IsItemDeactivatedAfterEdit()) updated = true;
        }
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

        ImGui::SliderFloat("Metallic Factor", &material.metallicFactor, 0.0f, 1.0f);
        if (ImGui::IsItemDeactivatedAfterEdit()) updated = true;

        ImGui::SliderFloat("Roughness Factor", &material.roughnessFactor, 0.0f, 1.0f);
        if (ImGui::IsItemDeactivatedAfterEdit()) updated = true;
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

    if (emmisiveTexture.textureID != 0)
    {
        ImGui::Text("Emissive Texture");
        ImGui::Image((ImTextureID)(intptr_t)emmisiveTexture.textureID, ImVec2(256, 256));
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Texture Dimensions: %d, %d", emmisiveTexture.width, emmisiveTexture.height);
        }
        // TODO: commented all select buttons until save data to meta is implemented
        /*ImGui::SameLine();
        if (ImGui::Button("Select Emissive Texture"))
        {
            ImGui::OpenPopup(CONSTANT_TEXTURE_SELECT_DIALOG_ID);
        }

        if (ImGui::IsPopupOpen(CONSTANT_TEXTURE_SELECT_DIALOG_ID))
        {
            UID handle = ChangeTexture(
                App->GetEditorUIModule()->RenderResourceSelectDialog<UID>(
                    CONSTANT_TEXTURE_SELECT_DIALOG_ID, App->GetLibraryModule()->GetTextureMap(), INVALID_UID
                ),
                emmisiveTexture, material.emmisiveTex
            );

            if (handle != NULL)
            {
                material.emmisiveTex = handle;
                updated              = true;
            }
        }*/
    }

    // Do this to avoid saving each frame you are editing the material
    if (!updated && wasUpdated)
    {
        SaveToMeta();
        App->GetSceneModule()->GetScene()->UpdateAllMaterialInstances(uid);
    }

    wasUpdated = updated;
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

                rapidjson::Value diffuseColor(rapidjson::kArrayType);
                diffuseColor.PushBack(material.diffColor[0], allocator);
                diffuseColor.PushBack(material.diffColor[1], allocator);
                diffuseColor.PushBack(material.diffColor[2], allocator);
                diffuseColor.PushBack(material.diffColor[3], allocator);

                importOptions.AddMember("diffuseColor", diffuseColor, allocator);
                importOptions.AddMember("metallicFactor", material.metallicFactor, allocator);
                importOptions.AddMember("roughnessFactor", material.roughnessFactor, allocator);

                importOptions.AddMember("isTransparent", isTransparent, allocator);
                importOptions.AddMember("isAlphaDiscard", isAlpha, allocator);
                importOptions.AddMember("isDoubleSided", doubleSided, allocator);
                importOptions.AddMember("applyWind", applyWind, allocator);
                importOptions.AddMember("vCoord0", vCoord0, allocator);
                importOptions.AddMember("vCoord1", vCoord1, allocator);
                importOptions.AddMember("useCentralPivot", useCentralPivot, allocator);
                importOptions.AddMember("useWindGravity", useWindGravity, allocator);
                importOptions.AddMember("windXAmplitude", windXAmplitude, allocator);
                importOptions.AddMember("windYAmplitude", windYAmplitude, allocator);
                importOptions.AddMember("windZAmplitude", windZAmplitude, allocator);
                importOptions.AddMember("windXFrequency", windXFrequency, allocator);
                importOptions.AddMember("windYFrequency", windYFrequency, allocator);
                importOptions.AddMember("windZFrequency", windZFrequency, allocator);
                importOptions.AddMember("windTimeScale", windTimeScale, allocator);

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

void ResourceMaterial::LoadMaterialData(const Material& mat, const rapidjson::Value& importOptions)
{
    // Prioitize updated values saved in meta. If they are missing, fall back on the ones in the original material

    if (importOptions.HasMember("defaultTextureUID") && importOptions["defaultTextureUID"].IsUint64())
        defaultTextureUID = importOptions["defaultTextureUID"].GetUint64();
    else defaultTextureUID = INVALID_UID;

    if (importOptions.HasMember("diffuseColor") && importOptions["diffuseColor"].IsArray())
    {
        const rapidjson::Value& diffuseColor = importOptions["diffuseColor"];
        material.diffColor[0]                = diffuseColor[0].GetFloat();
        material.diffColor[1]                = diffuseColor[1].GetFloat();
        material.diffColor[2]                = diffuseColor[2].GetFloat();
        material.diffColor[3]                = diffuseColor[3].GetFloat();
    }
    else
    {
        material.diffColor = mat.GetDiffuseFactor();
    }

    if (importOptions.HasMember("metallicFactor") && importOptions["metallicFactor"].IsFloat())
        material.metallicFactor = importOptions["metallicFactor"].GetFloat();
    else material.metallicFactor = mat.GetMetallicFactor();

    if (importOptions.HasMember("roughnessFactor") && importOptions["roughnessFactor"].IsFloat())
        material.roughnessFactor = importOptions["roughnessFactor"].GetFloat();
    else material.roughnessFactor = mat.GetRoughnessFactor();

    if (importOptions.HasMember("isTransparent") && importOptions["isTransparent"].IsBool())
        isTransparent = importOptions["isTransparent"].GetBool();
    else isTransparent = false;

    if (importOptions.HasMember("isAlphaDiscard") && importOptions["isAlphaDiscard"].IsBool())
        isAlpha = importOptions["isAlphaDiscard"].GetBool();
    else isAlpha = false;

    if (importOptions.HasMember("isDoubleSided") && importOptions["isDoubleSided"].IsBool())
        doubleSided = importOptions["isDoubleSided"].GetBool();
    else doubleSided = false;

    if (importOptions.HasMember("applyWind") && importOptions["applyWind"].IsBool())
        applyWind = importOptions["applyWind"].GetBool();
    else applyWind = false;

    if (importOptions.HasMember("vCoord0") && importOptions["vCoord0"].IsFloat())
        vCoord0 = importOptions["vCoord0"].GetFloat();
    else vCoord0 = 0.0f;

    if (importOptions.HasMember("vCoord1") && importOptions["vCoord1"].IsFloat())
        vCoord1 = importOptions["vCoord1"].GetFloat();
    else vCoord1 = 1.0f;

    if (importOptions.HasMember("useCentralPivot") && importOptions["useCentralPivot"].IsBool())
        useCentralPivot = importOptions["useCentralPivot"].GetBool();
    else useCentralPivot = false;

    if (importOptions.HasMember("useWindGravity") && importOptions["useWindGravity"].IsBool())
        useWindGravity = importOptions["useWindGravity"].GetBool();
    else useWindGravity = false;

    if (importOptions.HasMember("windXAmplitude") && importOptions["windXAmplitude"].IsFloat())
        windXAmplitude = importOptions["windXAmplitude"].GetFloat();
    else windXAmplitude = 1.0f;

    if (importOptions.HasMember("windYAmplitude") && importOptions["windYAmplitude"].IsFloat())
        windYAmplitude = importOptions["windYAmplitude"].GetFloat();
    else windYAmplitude = 1.0f;

    if (importOptions.HasMember("windZAmplitude") && importOptions["windZAmplitude"].IsFloat())
        windZAmplitude = importOptions["windZAmplitude"].GetFloat();
    else windZAmplitude = 1.0f;

    if (importOptions.HasMember("windXFrequency") && importOptions["windXFrequency"].IsFloat())
        windXFrequency = importOptions["windXFrequency"].GetFloat();
    else windXFrequency = 1.0f;

    if (importOptions.HasMember("windYFrequency") && importOptions["windYFrequency"].IsFloat())
        windYFrequency = importOptions["windYFrequency"].GetFloat();
    else windYFrequency = 1.0f;

    if (importOptions.HasMember("windZFrequency") && importOptions["windZFrequency"].IsFloat())
        windZFrequency = importOptions["windZFrequency"].GetFloat();
    else windZFrequency = 1.0f;

    if (importOptions.HasMember("windTimeScale") && importOptions["windTimeScale"].IsFloat())
        windTimeScale = importOptions["windTimeScale"].GetFloat();
    else windTimeScale = 1.0f;

    material.specColor           = mat.GetSpecularFactor();
    material.shininess           = mat.GetGlossinessFactor();
    material.shininessInAlpha    = false;

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
        hasNormal            = true;
    }

    ResourceTexture* occTexture = TextureImporter::LoadTexture(mat.GetOcclusionTexture());
    if (occTexture != nullptr)
    {
        // GLOG("%s has normal", normTexture->GetName().c_str());
        occlusionTexture.textureID = occTexture->GetTextureID();

        material.occlusionTex      = glGetTextureHandleARB(occTexture->GetTextureID());
        glMakeTextureHandleResidentARB(material.occlusionTex);

        occlusionTexture.width  = occTexture->GetTextureWidth();
        occlusionTexture.height = occTexture->GetTextureHeight();
    }

    ResourceTexture* emmTexture = TextureImporter::LoadTexture(mat.GetEmissiveTexture());
    if (emmTexture != nullptr)
    {
        // GLOG("%s has normal", normTexture->GetName().c_str());
        emmisiveTexture.textureID = emmTexture->GetTextureID();

        material.emmisiveTex      = glGetTextureHandleARB(emmTexture->GetTextureID());
        glMakeTextureHandleResidentARB(material.emmisiveTex);

        emmisiveTexture.width  = emmTexture->GetTextureWidth();
        emmisiveTexture.height = emmTexture->GetTextureHeight();
    }

    delete diffTexture;
    delete metallicRoughnessTexture;
    delete normTexture;
    delete occTexture;
    delete emmTexture;
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

    if (occlusionTexture.textureID != 0)
    {
        glMakeTextureHandleNonResidentARB(material.occlusionTex);
        glDeleteTextures(1, &occlusionTexture.textureID);
    }

    if (emmisiveTexture.textureID != 0)
    {
        glMakeTextureHandleNonResidentARB(material.emmisiveTex);
        glDeleteTextures(1, &emmisiveTexture.textureID);
    }
}

void ResourceMaterial::SetDiffColor(const float4& newColor)
{
    material.diffColor = newColor;
    // SaveToMeta();
    App->GetSceneModule()->GetScene()->UpdateAllMaterialInstances(uid);
}