#pragma once

#include "ParticleAddon.h"

class SpritesheetAddon : public ParticleAddon
{
  public:
    SpritesheetAddon(ParticleEmitter* owner);
    SpritesheetAddon(const rapidjson::Value& initialState, ParticleEmitter* owner);
    ~SpritesheetAddon() override;

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;

    void Init(EmitterInstance* emitterInstance);
    void Update(float deltaTime, EmitterInstance* emitterInstance) override;
    void RenderEditorInspector() override;

  public:
    float currentFrame = 0.f;
    int rows           = 1;
    int columns        = 1;

  private:
    float animationSpeed = 1.f;
    float timePerFrame   = 0.f;
    float playTime       = 0.f;

    int randomXTiles[2]  = {1, 1};
    int randomYTiles[2]  = {1, 1};

    bool randomizeOffset = false;
};
