#pragma once

#include "ParticleAddon.h"

class AreaAddon : public ParticleAddon
{
  public:
    AreaAddon(ParticleEmitter* owner);
    AreaAddon(const rapidjson::Value& initialState, ParticleEmitter* owner);
    ~AreaAddon() override;

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;

    void Init(EmitterInstance* emitterInstance);
    void Update(float deltaTime, EmitterInstance* emitterInstance) override;
    void RenderEditorInspector() override;

  private:
};
