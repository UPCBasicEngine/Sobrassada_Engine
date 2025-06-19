#pragma once

#include "ParticleAddon.h"

#include "Math/float4.h"

class ImGradient;

class ColorAddon : public ParticleAddon
{
  public:
    ColorAddon(ParticleEmitter* owner);
    ColorAddon(const rapidjson::Value& initialState, ParticleEmitter* owner);
    ~ColorAddon() override;

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;

    void Init(EmitterInstance* emitterInstance);
    void Update(float deltaTime, EmitterInstance* emitterInstance) override;
    void RenderEditorInspector() override;

  private:
    float4 particleColor = float4::one;

    ImGradient* gradient = nullptr;
};
