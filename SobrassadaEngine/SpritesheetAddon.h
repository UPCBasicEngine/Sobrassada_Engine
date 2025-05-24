#pragma once

#include "ParticleAddon.h"

class SpritesheetAddon : public ParticleAddon
{
  public:
    SpritesheetAddon();
    SpritesheetAddon(const rapidjson::Value& initialState);
    ~SpritesheetAddon();

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;

    void Init(EmitterInstance* emitterInstance);
    void Update(float deltaTime, EmitterInstance* emitterInstance) override;
    void RenderEditorInspector() override;

  private:
    int rows = 0;
    int columns = 0;
    float animationSpeed = 0;
};
