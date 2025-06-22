#include "ParticleSystemComponent.h"

#include "Application.h"
#include "EmitterInstance.h"
#include "ParticleSystem.h"
#include "ParticleSystemModule.h"

#include "imgui.h"

ParticleSystemComponent::ParticleSystemComponent(UID uid, GameObject* parent)
    : Component(uid, parent, "ParticleSystem", COMPONENT_PARTICLE_SYSTEM)
{
}

ParticleSystemComponent::ParticleSystemComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{

    if (initialState.HasMember("ParticleSystemTag"))
        particleSystemTag = HashString(initialState["ParticleSystemTag"].GetString());

    App->GetParticleModule()->ResquestParticleSystem(particleSystemTag, initialState, this);
}

ParticleSystemComponent::~ParticleSystemComponent()
{
    if (particleSystem) particleSystem->RemoveComponent(particleSystemIterator);
}

void ParticleSystemComponent::Init()
{
    for (auto& emitter : emitterInstances)
        emitter.Spawn();
}

void ParticleSystemComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);

    if (particleSystem) particleSystem->Save(targetState, allocator);
}

void ParticleSystemComponent::Clone(const Component* other)
{
    if (other->GetType() == ComponentType::COMPONENT_PARTICLE_SYSTEM)
    {
        const ParticleSystemComponent* otherParticles = static_cast<const ParticleSystemComponent*>(other);

        if (otherParticles->particleSystem)
            App->GetParticleModule()->ResquestParticleSystem(otherParticles->particleSystemTag, this);
    }
}

void ParticleSystemComponent::Update(float deltaTime)
{
    for (auto& emitter : emitterInstances)
        emitter.Update(deltaTime);
}

void ParticleSystemComponent::Render(float deltaTime)
{
}

void ParticleSystemComponent::RenderDebug(float deltaTime)
{
    if (currentEmitter) currentEmitter->RenderDebug(parent);
}

void ParticleSystemComponent::RenderEditorInspector()
{
    ImGui::InputText("Particle System Name", newParticleTagName, IM_ARRAYSIZE(newParticleTagName));

    if (ImGui::Button("Create Particle System"))
    {
        HashString requestedTag(newParticleTagName);
        App->GetParticleModule()->ResquestParticleSystem(requestedTag, this);
        memset(newParticleTagName, 0, sizeof(newParticleTagName));
    }
    ImGui::SameLine();
    if (ImGui::Button("Duplicate Particle System"))
    {
        HashString requestedTag(newParticleTagName);
        App->GetParticleModule()->DuplicateParticleSystem(requestedTag, this, particleSystemTag);
        memset(newParticleTagName, 0, sizeof(newParticleTagName));
    }

    ImGui::Text("Selected particle system:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.102f, 0.659f, 1.f, 1.f), particleSystemTag.GetString().c_str());

    if (ImGui::Button("Change tag"))
    {
        ImGui::OpenPopup("ParticleSystemSelection");
    }

    if (ImGui::BeginPopup("ParticleSystemSelection"))
    {
        auto& particleTags = App->GetParticleModule()->GetTags();

        if (ImGui::BeginListBox(
                "##ParticleSystemSelectionList",
                ImVec2(ImGui::CalcItemWidth(), ImGui::GetFrameHeightWithSpacing() * particleTags.size())
            ))
        {

            for (int i = 0; i < particleTags.size(); ++i)
            {
                if (ImGui::Selectable(particleTags[i].GetString().c_str()))
                {
                    if (particleTags[i] != particleSystemTag)
                    {
                        HashString selectedTag = particleTags[i];
                        App->GetParticleModule()->ResquestParticleSystem(selectedTag, this);
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
            ImGui::EndListBox();
        }
        ImGui::EndPopup();
    }

    if (ImGui::Button("Spawn all particles"))
    {
        for (auto& emitter : emitterInstances)
            emitter.Spawn();
    }

    if(ImGui::Button("Stop playing"))
    {
        StopInstances();
    }
    ImGui::SameLine();
    if (ImGui::Button("STOP ALL PLAYING !"))
    {
        App->GetParticleModule()->StopAllParticles();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (!particleSystem) return;

    ImGui::InputText("New Emitter name", newEmitterTagName, IM_ARRAYSIZE(newEmitterTagName));

    if (ImGui::Button("Create emitter"))
    {
        if (particleSystem)
        {
            particleSystem->AddEmitter(newEmitterTagName);
        }
        memset(newEmitterTagName, 0, sizeof(newEmitterTagName));
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::BeginCombo("Current emitter", currentEmitter ? currentEmitter->GetName().c_str() : "None"))
    {
        for (int i = 0; i < emitterInstances.size(); ++i)
        {
            if (ImGui::Selectable(emitterInstances[i].GetName().c_str())) currentEmitter = &emitterInstances[i];
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete current emmitter"))
    {
        if (currentEmitter && particleSystem) particleSystem->RemoveEmitter(currentEmitter->GetTag());
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (currentEmitter) currentEmitter->RenderEditor();
}

void ParticleSystemComponent::ReloadEmitterInstances(
    const std::vector<std::pair<HashString, ParticleEmitter*>>& emitters
)
{
    emitterInstances.clear();
    emitterInstances.reserve(emitters.size());

    for (auto& emitter : emitters)
    {
        emitterInstances.push_back(EmitterInstance(emitter.second, this));
    }

    if (emitterInstances.size() > 0) currentEmitter = &emitterInstances[0];
}

void ParticleSystemComponent::StopInstances()
{
    for (auto& emitter : emitterInstances)
        emitter.Stop();
}

void ParticleSystemComponent::SetParticleSystem(ParticleSystem* newParticleSystem)
{
    particleSystem    = newParticleSystem;
    particleSystemTag = newParticleSystem->GetTag();
}
