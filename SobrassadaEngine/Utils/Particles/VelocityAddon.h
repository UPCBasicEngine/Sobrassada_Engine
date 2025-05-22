#pragma once

#include "ParticleAddon.h"

class VelocityAddon : public ParticleAddon
{
  public:
    VelocityAddon(ParticleEmitter* owner);
    VelocityAddon(const rapidjson::Value& initialState, ParticleEmitter* owner);
    ~VelocityAddon() override;

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;

    void Init();
    void Update(float deltaTime, EmitterInstance* emitterInstance) override;
    void RenderEditorInspector() override;

  private:
    float startSpeed = 1.f;
};
