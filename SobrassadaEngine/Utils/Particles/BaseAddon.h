#pragma once

#include "ParticleAddon.h"

#include "Math/float2.h"

class BaseAddon : public ParticleAddon
{
  public:
    BaseAddon(ParticleEmitter* owner);
    BaseAddon(const rapidjson::Value& initialState, ParticleEmitter* owner);
    ~BaseAddon() override;

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;

    void Init(EmitterInstance* emitterInstance);
    void Update(float deltaTime, EmitterInstance* emitterInstance) override;
    void RenderEditorInspector() override;

  private:
    float currentEmissionTime = 0.f;
    float duration            = 5.f;
    bool loop                 = false;
    int maxParticles          = 100;

    bool randomLifetime       = false;
    float minLifetime         = 0.5f;
    float maxLifetime         = 3.f;

    bool randomizeSizeX       = false;
    bool randomizeSizeY       = false;
    float2 sizeValuesX        = float2(0, 1);
    float2 sizeValuesY        = float2(0, 1);

    bool randomRotation       = false;
    float2 rotation           = float2::zero;

    bool useSizeCurveX        = false;
    bool useSizeCurveY        = false;
    float sizeBezierX[5]      = {0.f, 0.f, 1.f, 1.f};
    float sizeBezierY[5]      = {0.f, 0.f, 1.f, 1.f};

    int particlesPerSecond    = 15;
    float spawnDeltaTime      = 0.f;
};
