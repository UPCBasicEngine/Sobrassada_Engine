#include "ParticleSystemComponent.h"

#include "Application.h"
#include "ParticleEmitter.h"
#include "ParticleSystemModule.h"

#include "imgui.h"

ParticleSystemComponent::ParticleSystemComponent(UID uid, GameObject* parent)
    : Component(uid, parent, "ParticleSystem", COMPONENT_PARTICLE_SYSTEM)
{
}

ParticleSystemComponent::ParticleSystemComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    if (initialState.HasMember("Emitters") && initialState["Emitters"].IsArray())
    {
        const rapidjson::Value& jsonEmitters = initialState["Emitters"];

        for (rapidjson::SizeType i = 0; i < jsonEmitters.Size(); i++)
        {
            const rapidjson::Value& newEmitterJSON = jsonEmitters[i];

            ParticleEmitter* newEmitter = App->GetParticleModule()->RequestParticleEmitter(newEmitterJSON, this);

            emitters.push_back({newEmitter->GetName(), newEmitter});
        }

        if (emitters.size() > 0 && emitters[0].second != nullptr) currentEmitter = emitters[0].second;
    }
}

ParticleSystemComponent::~ParticleSystemComponent()
{
    for (auto emitter : emitters)
    {
        if (emitter.second) App->GetParticleModule()->DeleteParticleEmitter(emitter.second->GetUID());
    }
}

void ParticleSystemComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);

    rapidjson::Value emittersArrayJSON(rapidjson::kArrayType);

    // THIS WILL GO IN A LOOP THROUGH A VECTOR
    for (auto emitter : emitters)
    {
        rapidjson::Value currentEmitterJSON(rapidjson::kObjectType);
        if (emitter.second) emitter.second->Save(currentEmitterJSON, allocator);
        emittersArrayJSON.PushBack(currentEmitterJSON, allocator);
    }

    targetState.AddMember("Emitters", emittersArrayJSON, allocator);
}

void ParticleSystemComponent::Clone(const Component* other)
{
}

void ParticleSystemComponent::Update(float deltaTime)
{
}

void ParticleSystemComponent::Render(float deltaTime)
{
}

void ParticleSystemComponent::RenderDebug(float deltaTime)
{
}

void ParticleSystemComponent::RenderEditorInspector()
{
    ImGui::InputText("New Emitter name", newTagName, IM_ARRAYSIZE(newTagName));

    if (ImGui::Button("Create emitter"))
    {
        currentEmitter = App->GetParticleModule()->RequestParticleEmitter(newTagName, this);
        emitters.push_back({currentEmitter->GetName(), currentEmitter});
        memset(newTagName, 0, sizeof(newTagName));
    } 

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::BeginCombo("Current emitter", currentEmitter ? currentEmitter->GetName().c_str() : "None"))
    {
        for (int i = 0; i < emitters.size(); ++i)
        {
            if (ImGui::Selectable(emitters[i].second->GetName().c_str())) currentEmitter = emitters[i].second;
        }
        ImGui::EndCombo();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (currentEmitter) currentEmitter->RenderEditor();
}
