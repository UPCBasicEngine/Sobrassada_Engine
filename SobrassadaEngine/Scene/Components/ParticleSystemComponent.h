#pragma once

#include "Component.h"

#include <utility>
#include <vector>

class ResourceMaterial;
class ResourceTexture;
class ParticleEmitter;

class ParticleSystemComponent : public Component
{
  public:
    ParticleSystemComponent(UID uid, GameObject* parent);
    ParticleSystemComponent(const rapidjson::Value& initialState, GameObject* parent);
    ~ParticleSystemComponent() override;

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;

    void Update(float deltaTime) override;
    void Render(float deltaTime) override;
    void RenderDebug(float deltaTime) override;
    void RenderEditorInspector() override;

  private:
    ParticleEmitter* currentEmitter = nullptr;
    std::vector<std::pair<std::string, ParticleEmitter*>> emitters;

    char newTagName[64]               = "";
};
