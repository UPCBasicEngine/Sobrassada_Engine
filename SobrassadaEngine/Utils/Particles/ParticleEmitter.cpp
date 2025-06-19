#include "ParticleEmitter.h"

#include "Application.h"
#include "AreaAddon.h"
#include "BaseAddon.h"
#include "ColorAddon.h"
#include "EditorUIModule.h"
#include "EmitterInstance.h"
#include "LibraryModule.h"
#include "OpenGLModule.h"
#include "ParticleSystem.h"
#include "ParticleSystemComponent.h"
#include "ResourceMaterial.h"
#include "ResourceTexture.h"
#include "ResourcesModule.h"
#include "ShaderModule.h"
#include "SpritesheetAddon.h"
#include "VelocityAddon.h"

#include "CameraModule.h"

#include "Math/float2.h"
#include "glew.h"
#include "imgui.h"
#include <chrono>

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

// REMOVE COMPONENT
template <std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
RemoveAddonTuple(std::tuple<Tp...>& tuple, ParticleAddonType selectedType, ParticleEmitter* parent)
{
}

template <std::size_t I = 0, typename... Tp>
    inline typename std::enable_if <
    I<sizeof...(Tp), void>::type
    RemoveAddonTuple(std::tuple<Tp...>& tuple, ParticleAddonType selectedType, ParticleEmitter* parent)
{
    if (std::get<I>(tuple) && std::get<I>(tuple)->GetType() == selectedType)
    {
        delete std::get<I>(tuple);
        std::get<I>(tuple) = nullptr;
        parent->SetAddonDeleted((int)selectedType - 1);
        return;
    }
    RemoveAddonTuple<I + 1, Tp...>(tuple, selectedType, parent);
}
// ---------- END SECTION FOR TUPLE ITERATION ----------

ParticleEmitter::ParticleEmitter(const HashString& tag, ParticleSystem* owner) : emitterTag(tag), owner(owner)
{
    addonTuple = std::make_tuple(ADDON_NULLPTR);
    createdAddons.reset();
    ParticleUtils::CreateEmptyParticleAddon(ParticleAddonType::BASE, this);
}

ParticleEmitter::ParticleEmitter(const rapidjson::Value& initialState, ParticleSystem* owner) : owner(owner)
{
    addonTuple = std::make_tuple(ADDON_NULLPTR);
    createdAddons.reset();

    if (initialState.HasMember("EmitterTag")) emitterTag = HashString(initialState["EmitterTag"].GetString());
    if (initialState.HasMember("UseTexture")) useTexture = initialState["UseTexture"].GetBool();

    if (initialState.HasMember("Material"))
    {
        UID materialUID = initialState["Material"].GetUint64();
        UpdateMaterial(materialUID);
    }

    if (initialState.HasMember("Texture"))
    {
        UID textureUID = initialState["Texture"].GetUint64();
        UpdateTexture(textureUID);
    }

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

    if (initialState.HasMember("RenderPriority")) renderPriority = initialState["RenderPriority"].GetInt();
    if (initialState.HasMember("blendingMode"))
        blendingMode = EmitterBlendingMode(initialState["blendingMode"].GetInt());
    if (initialState.HasMember("colorIntensity")) colorIntensity = initialState["colorIntensity"].GetFloat();
}

ParticleEmitter::~ParticleEmitter()
{
    std::apply([](auto&... tupleVar) { ((delete tupleVar, tupleVar = nullptr), ...); }, addonTuple);

    glDeleteBuffers(1, &particlesVBO);
    glDeleteBuffers(1, &particleTileOffsetVBO);
    glDeleteBuffers(1, &particleColorsVBO);
    glDeleteBuffers(1, &particleSizeVBO);
    glDeleteBuffers(1, &particleRotationVBO);
}

void ParticleEmitter::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{

    targetState.AddMember("EmitterTag", rapidjson::Value(emitterTag.GetString().c_str(), allocator), allocator);
    targetState.AddMember("UseTexture", useTexture, allocator);

    targetState.AddMember("Material", material != nullptr ? material->GetUID() : INVALID_UID, allocator);
    targetState.AddMember("Texture", texture != nullptr ? texture->GetUID() : INVALID_UID, allocator);

    rapidjson::Value addonsJSON(rapidjson::kArrayType);
    SaveAddonsTuple(addonTuple, addonsJSON, allocator);

    targetState.AddMember("Addons", addonsJSON, allocator);
    targetState.AddMember("RenderPriority", renderPriority, allocator);
    targetState.AddMember("blendingMode", (int)blendingMode, allocator);
    targetState.AddMember("colorIntensity", colorIntensity, allocator);
}

void ParticleEmitter::Update(float deltaTime, EmitterInstance* emitterInstance)
{
    if (!emitterInstance->isEmitting) return;

    std::apply(
        [deltaTime, emitterInstance](auto&... pointer)
        { ((pointer ? pointer->Update(deltaTime, emitterInstance) : Nothing()), ...); }, addonTuple
    );

    UpdateParticlesVBO(emitterInstance);
}

void ParticleEmitter::Spawn(EmitterInstance* emitterInstance)
{
    std::apply(
        [emitterInstance](auto&... pointer) { ((pointer ? pointer->Init(emitterInstance) : Nothing()), ...); },
        addonTuple
    );
}

// TEMPORAL, PROBABLY CAN SEND ACTIVES PARTICLES TO WHOLE BATCH OF EMITTERS THAT SHARE SAME TEXTURE
void ParticleEmitter::RenderParticles(const float4x4& VP, const float3& rightVector, const float3& upVector)
{
    if (batchedParticles.size() < 1) return;

    if ((useTexture ? texture != nullptr : material != nullptr) && quadVBO && particlesVBO && particleTileOffsetVBO)
    {
        const auto start        = std::chrono::high_resolution_clock::now();

        float4x4 viewProjection = VP;
        float3 cameraRight      = rightVector;
        float3 cameraUp         = upVector;
        float2 tileSize         = float2(1, 1);

        glUseProgram(App->GetShaderModule()->GetParticleSystemProgram());

        switch (blendingMode)
        {
        case EmitterBlendingMode::ALPHA:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case EmitterBlendingMode::ALPHA_ADDITIVE:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            break;
        case EmitterBlendingMode::ADDITIVE:
            glBlendFunc(GL_ONE, GL_ONE);
            break;
        default:
            break;
        }

        glUniform3fv(0, 1, &cameraRight[0]);
        glUniform3fv(1, 1, &cameraUp[0]);
        glUniformMatrix4fv(2, 1, GL_TRUE, &viewProjection[0][0]);

        if (useSpritesheet)
        {
            SpritesheetAddon* spritesheet = std::get<SpritesheetAddon*>(addonTuple);
            tileSize                      = float2((float)spritesheet->rows, (float)spritesheet->columns);
            glUniform1f(3, spritesheet->currentFrame);
        }

        glUniform2fv(4, 1, &tileSize[0]);
        glUniform1f(5, colorIntensity);

        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);

        // Sending vertex coords
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // Sending texture coordiantes
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float) * 3 * 6));

        // Sending center billboard positions
        glBindBuffer(GL_ARRAY_BUFFER, particlesVBO);

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glVertexAttribDivisor(2, 1);

        // Sending particle offset
        glBindBuffer(GL_ARRAY_BUFFER, particleTileOffsetVBO);

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 2, GL_INT, GL_FALSE, 2 * sizeof(int), (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glVertexAttribDivisor(3, 1);

        // Sending particle colors
        glBindBuffer(GL_ARRAY_BUFFER, particleColorsVBO);

        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glVertexAttribDivisor(4, 1);

        // Sending particle size
        glBindBuffer(GL_ARRAY_BUFFER, particleSizeVBO);

        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glVertexAttribDivisor(5, 1);

        // Sending particle rotation
        glBindBuffer(GL_ARRAY_BUFFER, particleRotationVBO);

        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glVertexAttribDivisor(6, 1);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, useTexture ? texture->GetTextureID() : material->GetDiffuseColorID());

        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, (GLsizei)batchedParticles.size());

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        const auto end                             = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<float> elapsed = end - start;

        unsigned int totalTrangles                 = (unsigned int)batchedParticles.size() * 2;

        App->GetOpenGLModule()->AddTrianglesPerSecond(totalTrangles / elapsed.count());
        App->GetOpenGLModule()->AddVerticesCount(totalTrangles * 3);
        App->GetOpenGLModule()->AddDrawCallsCount();

        alivePositions.clear();
        tileOffsets.clear();
        batchedParticles.clear();
        particleColors.clear();
        particleSize.clear();
        particleRotation.clear();
    }
}

void ParticleEmitter::RenderEditor()
{
    ImGui::SameLine();

    if (ImGui::Button("Manage Addons"))
    {
        ImGui::OpenPopup("RenderAddonsManager");
    }

    if (ImGui::BeginPopup("RenderAddonsManager"))
    {
        float listBoxSize = (float)AddonTypeStringsSize + 0.5f;
        if (ImGui::BeginListBox(
                "##RenderAddonsManagerList",
                ImVec2(ImGui::CalcItemWidth(), ImGui::GetFrameHeightWithSpacing() * listBoxSize)
            ))
        {
            for (int i = 2; i < AddonTypeStringsSize; ++i)
            {
                bool currentBitValue = createdAddons[i - 1];
                if (ImGui::Checkbox(AddonTypeStrings[i], &currentBitValue))
                {
                    if (currentBitValue) AddAddon(ParticleAddonType(i));
                    else RemoveAddon(ParticleAddonType(i));
                }
            }

            ImGui::EndListBox();
        }

        ImGui::EndPopup();
    }

    ImGui::SameLine();
    ImGui::PushItemWidth(150);
    if (ImGui::InputInt("Render Priority", &renderPriority))
    {
        owner->SortEmitters();
    }

    ImGui::Spacing();

    if (ImGui::BeginCombo("Blending mode", EmitterBlendingModeStrings[(int)blendingMode]))
    {
        for (int i = 0; i < EmitterBlendingModeStringsSize; ++i)
        {
            if (ImGui::Selectable(EmitterBlendingModeStrings[i])) blendingMode = EmitterBlendingMode(i);
        }

        ImGui::EndCombo();
    }

    ImGui::SameLine();
    if (ImGui::DragFloat("Color intensity", &colorIntensity, 0.005f, 0.f, 2.f))
    {
        if (colorIntensity < 0.f) colorIntensity = 0.f;
    }

    ImGui::Spacing();

    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Material / Texture
    if (ImGui::CollapsingHeader("Resource"))
    {
        if (ImGui::BeginCombo("Resource type", ResourceTypeStrings[useTexture ? 1 : 0]))
        {
            for (int i = 0; i < ResourceTypeStringsSize; ++i)
            {
                if (ImGui::Selectable(ResourceTypeStrings[i])) useTexture = i;
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
                    ImGui::SetTooltip(
                        "Texture Dimensions: %d, %d", texture->GetTextureWidth(), texture->GetTextureWidth()
                    );
                }
            }
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    std::apply([](auto&... pointer) { ((pointer ? pointer->RenderEditorInspector() : Nothing()), ...); }, addonTuple);
}

void ParticleEmitter::RenderDebug(GameObject* parent)
{
    std::apply([parent](auto&... pointer) { ((pointer ? pointer->RenderDebug(parent) : Nothing()), ...); }, addonTuple);
}

void ParticleEmitter::AddAddon(ParticleAddonType type)
{
    if (!createdAddons[(int)type - 1]) ParticleUtils::CreateEmptyParticleAddon(type, this);
}

void ParticleEmitter::RemoveAddon(ParticleAddonType type)
{
    if (createdAddons[(int)type - 1])
    {
        RemoveAddonTuple(addonTuple, type, this);
    }
}

void ParticleEmitter::Stop()
{
    owner->Stop();
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
        App->GetResourcesModule()->ReleaseResource(texture);
        texture = newTexture;
    }
}

void ParticleEmitter::UpdateParticlesVBO(EmitterInstance* emitterInstance)
{

    batchedParticles.reserve(batchedParticles.size() + emitterInstance->particles.size());
    for (int i = 0; i < emitterInstance->particles.size(); ++i)
    {
        if (emitterInstance->particles[i].alive) batchedParticles.push_back(emitterInstance->particles[i]);
    }

    float3 cameraPosition = App->GetCameraModule()->GetCameraPosition();
    std::sort(
        batchedParticles.begin(), batchedParticles.end(),
        [&cameraPosition](const Particle& a, const Particle& b)
        {
            float distanceA = (a.position - cameraPosition).LengthSq();
            float distanceB = (b.position - cameraPosition).LengthSq();

            return distanceA > distanceB;
        }
    );

    // ADD AND RESERVE SPACE FOR PARTICLE ARRAY INSTANCES
    alivePositions.reserve(batchedParticles.size());
    tileOffsets.reserve(batchedParticles.size());
    particleColors.reserve(batchedParticles.size());
    particleSize.reserve(batchedParticles.size());
    particleRotation.reserve(batchedParticles.size());

    for (int i = 0; i < batchedParticles.size(); ++i)
    {
        alivePositions.push_back(batchedParticles[i].position);
        tileOffsets.push_back(batchedParticles[i].tileOffset);
        particleColors.push_back(batchedParticles[i].color);
        particleSize.push_back(batchedParticles[i].size);
        particleRotation.push_back(batchedParticles[i].rotation);
    }

    if (particlesVBO == 0) glGenBuffers(1, &particlesVBO);
    if (particleTileOffsetVBO == 0) glGenBuffers(1, &particleTileOffsetVBO);
    if (particleColorsVBO == 0) glGenBuffers(1, &particleColorsVBO);
    if (particleSizeVBO == 0) glGenBuffers(1, &particleSizeVBO);
    if (particleRotationVBO == 0) glGenBuffers(1, &particleRotationVBO);
    if (batchedParticles.size() > 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, particlesVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float3) * alivePositions.size(), &alivePositions[0], GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ARRAY_BUFFER, particleTileOffsetVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(int) * 2 * tileOffsets.size(), &tileOffsets[0], GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ARRAY_BUFFER, particleColorsVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float4) * particleColors.size(), &particleColors[0], GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ARRAY_BUFFER, particleSizeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float2) * particleSize.size(), &particleSize[0], GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ARRAY_BUFFER, particleRotationVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * particleRotation.size(), &particleRotation[0], GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}
