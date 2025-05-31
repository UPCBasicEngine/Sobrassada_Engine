#pragma once

#include "ParticleAddon.h"

#include "Math/float2.h"

constexpr const char* VelocityAddonStrings[] = {"Fixed values", "Curve interpolation"};
constexpr int VelocityAddonStringsSize       = sizeof(VelocityAddonStrings) / sizeof(char*);

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

    float2 xSpeed        = float2::one;
    float2 ySpeed        = float2::one;
    float2 zSpeed        = float2::one;

    bool useCurves       = false;

    float bezierX[5]     = {0.390f, 0.575f, 0.565f, 1.000f};
    float bezierY[5]     = {0.390f, 0.575f, 0.565f, 1.000f};
    float bezierZ[5]     = {0.390f, 0.575f, 0.565f, 1.000f};
};
