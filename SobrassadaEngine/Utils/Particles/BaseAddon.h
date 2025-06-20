#pragma once

#include "ParticleAddon.h"

#include "Math/float2.h"
#include "imgui.h"

struct Particle;

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
    void ResetCurveEditorPoints(ImVec2* pointsToReset);
    void InitializeParticleSize(Particle& particle);
    void UpdateParticleSize(Particle& particle, float valueOverLifetime);

  private:
    float currentEmissionTime                    = 0.f;
    float duration                               = 5.f;
    bool loop                                    = false;
    bool respawnLoop                             = false;
    int maxParticles                             = 100;

    bool randomLifetime                          = false;
    float minLifetime                            = 0.5f;
    float maxLifetime                            = 3.f;

    bool randomRotation                          = false;
    float2 rotation                              = float2::zero;

    bool updateXYApart                           = true;

    ParticleInterpolationType sizeInterpolation  = ParticleInterpolationType::FIXED_VALUES;
    ParticleInterpolationType sizeInterpolationX = ParticleInterpolationType::FIXED_VALUES;
    ParticleInterpolationType sizeInterpolationY = ParticleInterpolationType::FIXED_VALUES;

    bool randomizeSizeCombined                   = false;
    bool randomizeSizeX                          = false;
    bool randomizeSizeY                          = false;
    float2 combinedSize                          = {0.f, 1.f};
    float2 sizeValuesX                           = float2(0, 1);
    float2 sizeValuesY                           = float2(0, 1);

    float sizeBezierCombined[5]                  = {0.f, 0.f, 1.f, 1.f};
    float sizeBezierX[5]                         = {0.f, 0.f, 1.f, 1.f};
    float sizeBezierY[5]                         = {0.f, 0.f, 1.f, 1.f};

    // For same X Y size
    ImVec2 curveEditorPoints[MaxCurveEditorPoints];
    int curveEditorIndex         = -1;
    float2 curveEditorValueRange = {0.f, 1.f};

    ImVec2 curveEditorXPoints[MaxCurveEditorPoints];
    int curveEditorIndexX = -1;
    ImVec2 curveEditorYPoints[MaxCurveEditorPoints];
    int curveEditorIndexY  = -1;

    int particlesPerSecond = 15;
    float spawnDeltaTime   = 0.f;

    bool burst             = false;
};
