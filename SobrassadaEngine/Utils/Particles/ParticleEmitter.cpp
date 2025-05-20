#include "ParticleEmitter.h"

#include "BaseAddon.h"
#include "ParticleSystemComponent.h"
#include "ResourcesModule.h"
#include "VelocityAddon.h"
#include "Application.h"
#include "EditorUIModule.h"
#include "LibraryModule.h"
#include "ResourceMaterial.h"
#include "ResourceTexture.h"

#include "imgui.h"

// ---------- SECTION FOR TUPLE ITERATION ----------

// (Used in std apply for being able to use the ternary operator for nullptr's)
static void Nothing()
{
}

// SAVE ADDONS
template <std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type SaveAddonsTuple(
    const std::tuple<Tp...>& tuple, rapidjson::Value& addonsJSON, rapidjson::Document::AllocatorType& allocator
)
{
}

template <std::size_t I = 0, typename... Tp>
    inline typename std::enable_if <
    I<sizeof...(Tp), void>::type SaveAddonsTuple(
        const std::tuple<Tp...>& tuple, rapidjson::Value& addonsJSON, rapidjson::Document::AllocatorType& allocator
    )
{
    if (std::get<I>(tuple))
    {
        rapidjson::Value addonJSON(rapidjson::kObjectType);

        std::get<I>(tuple)->Save(addonJSON, allocator);

        addonsJSON.PushBack(addonJSON, allocator);
    }
    SaveAddonsTuple<I + 1, Tp...>(tuple, addonsJSON, allocator);
}

// ---------- END SECTION FOR TUPLE ITERATION ----------

ParticleEmitter::ParticleEmitter(UID uid, const std::string& name, ParticleSystemComponent* owner)
    : uid(uid), name(name), owner(owner)
{
    addonTuple = std::make_tuple(ADDON_NULLPTR);
    createdAddons.reset();
    ParticleUtils::CreateEmptyParticleAddon(ParticleAddonType::BASE, this);
}

ParticleEmitter::ParticleEmitter(const rapidjson::Value& initialState, ParticleSystemComponent* owner) : owner(owner)

{
    addonTuple = std::make_tuple(ADDON_NULLPTR);
    createdAddons.reset();

    if (initialState.HasMember("UID")) uid = initialState["UID"].GetUint64();
    if (initialState.HasMember("Name")) name = initialState["Name"].GetString();
    if (initialState.HasMember("UseTexture")) useTexture = initialState["UseTexture"].GetBool();

    if (initialState.HasMember("Addons") && initialState["Addons"].IsArray())
    {
        const rapidjson::Value& jsonAddons = initialState["Addons"];

        for (rapidjson::SizeType i = 0; i < jsonAddons.Size(); i++)
        {
            const rapidjson::Value& newAddonJSON = jsonAddons[i];

            ParticleAddonType type               = ParticleAddonType(newAddonJSON["AddonType"].GetInt());
            ParticleUtils::CreateExistingComponent(newAddonJSON, this);
        }
    }
}

ParticleEmitter::~ParticleEmitter()
{
    std::apply([](auto&... tupleVar) { ((delete tupleVar, tupleVar = nullptr), ...); }, addonTuple);
}

void ParticleEmitter::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{

    targetState.AddMember("UID", uid, allocator);
    targetState.AddMember("Name", rapidjson::Value(name.c_str(), allocator), allocator);
    targetState.AddMember("UseTexture", useTexture, allocator);

    rapidjson::Value addonsJSON(rapidjson::kArrayType);
    SaveAddonsTuple(addonTuple, addonsJSON, allocator);

    targetState.AddMember("Addons", addonsJSON, allocator);
}

void ParticleEmitter::Update(float deltaTime)
{
}

void ParticleEmitter::Spawn()
{
}

void ParticleEmitter::RenderEditor()
{
    ImGui::Text("Selected emitter: %s", name.c_str());
    if (ImGui::Button("Create Velocity")) AddAddon(ParticleAddonType::VELOCITY);

    if (ImGui::BeginCombo("Resource type", ResourceTypeStrings[useTexture ? 1 : 0]))
    {
        for (int i = 0; i < ResourceTypeStringsSize; ++i)
        {
            if (ImGui::Selectable(ResourceTypeStrings[i]))
            {
                useTexture = i;
                // App->GetBillboardModule()->UpdateTagUseTexture(billboardTag, useTexture);
            }
        }
        ImGui::EndCombo();
    }

    if (!useTexture)
    {
        if (ImGui::Button("Select material"))
        {
            ImGui::OpenPopup(CONSTANT_MATERIAL_SELECT_DIALOG_ID);
        }

        if (ImGui::IsPopupOpen(CONSTANT_MATERIAL_SELECT_DIALOG_ID))
        {

            const UID chosenMatUID = App->GetEditorUIModule()->RenderResourceSelectDialog<UID>(
                CONSTANT_MATERIAL_SELECT_DIALOG_ID, App->GetLibraryModule()->GetMaterialMap(), INVALID_UID
            );

            if (chosenMatUID != INVALID_UID) UpdateMaterial(chosenMatUID);
        }

        if (material != nullptr) material->OnEditorUpdate();
    }
    else
    {
        if (ImGui::Button("Select texture"))
        {
            ImGui::OpenPopup(CONSTANT_TEXTURE_SELECT_DIALOG_ID);
        }

        if (ImGui::IsPopupOpen(CONSTANT_TEXTURE_SELECT_DIALOG_ID))
        {

            const UID chosenTexUID = App->GetEditorUIModule()->RenderResourceSelectDialog<UID>(
                CONSTANT_TEXTURE_SELECT_DIALOG_ID, App->GetLibraryModule()->GetTextureMap(), INVALID_UID
            );

            if (chosenTexUID != INVALID_UID) UpdateTexture(chosenTexUID);
        }

        if (texture != nullptr)
        {
            ImGui::Text("Diffuse Texture");
            ImGui::Image((ImTextureID)(intptr_t)texture->GetTextureID(), ImVec2(256, 256));
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Texture Dimensions: %d, %d", texture->GetTextureWidth(), texture->GetTextureWidth()
                );
            }
        }
    }

    std::apply([](auto&... pointer) { ((pointer ? pointer->RenderEditorInspector() : Nothing()), ...); }, addonTuple);
}

void ParticleEmitter::AddAddon(ParticleAddonType type)
{
    if (!createdAddons[(int)type - 1]) ParticleUtils::CreateEmptyParticleAddon(type, this);
}

void ParticleEmitter::RemoveAddon(ParticleAddonType type)
{
}

void ParticleEmitter::UpdateMaterial(UID newMaterialUID)
{
    if (newMaterialUID == INVALID_UID || App->GetResourcesModule()->RequestResource(newMaterialUID) == nullptr)
    {
        newMaterialUID = DEFAULT_MATERIAL_UID;
    }

    if (material != nullptr && material->GetUID() == newMaterialUID) return;

    ResourceMaterial* newMaterial =
        dynamic_cast<ResourceMaterial*>(App->GetResourcesModule()->RequestResource(newMaterialUID));

    if (newMaterial != nullptr)
    {
        useTexture = false;

        App->GetResourcesModule()->ReleaseResource(material);
        material = newMaterial;
    }
}

void ParticleEmitter::UpdateTexture(UID newTextureUID)
{
    if (newTextureUID == INVALID_UID || App->GetResourcesModule()->RequestResource(newTextureUID) == nullptr)
    {
        newTextureUID = FALLBACK_TEXTURE_UID;
    }

    if (texture != nullptr && texture->GetUID() == newTextureUID) return;

    ResourceTexture* newTexture =
        dynamic_cast<ResourceTexture*>(App->GetResourcesModule()->RequestResource(newTextureUID));

    if (newTexture != nullptr)
    {
        useTexture = true;

        App->GetResourcesModule()->ReleaseResource(texture);
        texture = newTexture;
    }
}
