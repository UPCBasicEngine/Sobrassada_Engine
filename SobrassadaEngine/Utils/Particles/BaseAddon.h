#pragma once

#include "ParticleAddon.h"

class BaseAddon : ParticleAddon
{
  public:
    BaseAddon(ResourceEmitter* owner);
    BaseAddon(const rapidjson::Value& initialState, ResourceEmitter* owner);
    ~BaseAddon() override;

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;

    void Update(float deltaTime) const override;
    void RenderEditorInspector() override;

  private:
    float duration = 5.f;
    bool loop = false;
    unsigned int maxParticles = 100;
};
