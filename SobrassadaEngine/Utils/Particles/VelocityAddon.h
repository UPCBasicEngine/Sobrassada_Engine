#pragma once

#include "ParticleAddon.h"

#include "Math/float2.h"

class VelocityAddon : public ParticleAddon
{
  public:
    VelocityAddon(ParticleEmitter* owner);
    VelocityAddon(const rapidjson::Value& initialState, ParticleEmitter* owner);
    ~VelocityAddon() override;

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;

    void Init(EmitterInstance* emitterInstance);
    void Update(float deltaTime, EmitterInstance* emitterInstance) override;
    void RenderEditorInspector() override;

  private:
    bool randomizeXSpeed = false;
    bool randomizeYSpeed = false;
    bool randomizeZSpeed = false;

    float2 xSpeed        = float2::zero;
    float2 ySpeed        = float2::zero;
    float2 zSpeed        = float2::zero;

    bool useXCurve       = false;
    bool useYCurve       = false;
    bool useZCurve       = false;

    float bezierX[5]     = {0.f, 0.f, 1.f, 1.f};
    float bezierY[5]     = {0.f, 0.f, 1.f, 1.f};
    float bezierZ[5]     = {0.f, 0.f, 1.f, 1.f};

    bool gravity         = false;
    float gravityValue   = 0.f;
};
