#pragma once

#include "ParticleAddon.h"

#include "Math/float2.h"
#include "imgui.h"

struct Particle;

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
    void ResetCurveEditorPoints(ImVec2* pointsToReset);
    void InitializeParticleVelocity(Particle& particle);
    void UpdateParticleVelocity(Particle& particle, float valueOverLifetime);

  private:
    bool randomizeXSpeed                          = false;
    bool randomizeYSpeed                          = false;
    bool randomizeZSpeed                          = false;

    float2 xSpeed                                 = float2::zero;
    float2 ySpeed                                 = float2::zero;
    float2 zSpeed                                 = float2::zero;

    float bezierX[5]                              = {0.f, 0.f, 1.f, 1.f};
    float bezierY[5]                              = {0.f, 0.f, 1.f, 1.f};
    float bezierZ[5]                              = {0.f, 0.f, 1.f, 1.f};

    ParticleInterpolationType speedXInterpolation = ParticleInterpolationType::FIXED_VALUES;
    ParticleInterpolationType speedYInterpolation = ParticleInterpolationType::FIXED_VALUES;
    ParticleInterpolationType speedZInterpoaltion = ParticleInterpolationType::FIXED_VALUES;

    ImVec2 curveEditorPointsX[MaxCurveEditorPoints];
    int curveEditorIndexX = -1;

    ImVec2 curveEditorPointsY[MaxCurveEditorPoints];
    int curveEditorIndexY = -1;

    ImVec2 curveEditorPointsZ[MaxCurveEditorPoints];
    int curveEditorIndexZ = -1;

    bool gravity          = false;
    float gravityValue    = 0.f;
};
