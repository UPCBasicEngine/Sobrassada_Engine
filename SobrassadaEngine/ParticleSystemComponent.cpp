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

            emitters.push_back(App->GetParticleModule()->RequestParticleEmitter(newEmitterJSON, this));
        }

        if (emitters.size() > 0 && emitters[0] != nullptr) currentEmitter = emitters[0];
    }
}

ParticleSystemComponent::~ParticleSystemComponent()
{
    for (auto emitter : emitters)
    {
        if (emitter) App->GetParticleModule()->DeleteParticleEmitter(emitter->GetUID());
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
        if (emitter) emitter->Save(currentEmitterJSON, allocator);
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
        emitters.push_back(currentEmitter);
        memset(newTagName, 0, sizeof(newTagName));
    }

    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();

    if (currentEmitter) currentEmitter->RenderEditor();
}
