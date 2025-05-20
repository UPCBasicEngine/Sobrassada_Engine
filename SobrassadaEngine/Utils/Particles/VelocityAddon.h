#pragma once

#include "ParticleAddon.h"

class VelocityAddon : ParticleAddon
{
  public:
    VelocityAddon(ResourceEmitter* owner);
    VelocityAddon(const rapidjson::Value& initialState, ResourceEmitter* owner);
    ~VelocityAddon() override;

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;

    void Init();
    void Update(float deltaTime) const override;
    void RenderEditorInspector() override;

  private:
    float startSpeed = 1.f;
};
