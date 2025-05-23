#pragma once

#include "ParticleAddon.h"

class BaseAddon : public ParticleAddon
{
  public:
    BaseAddon();
    BaseAddon(const rapidjson::Value& initialState);
    ~BaseAddon() override;

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;

    void Init(EmitterInstance* emitterInstance);
    void Update(float deltaTime, EmitterInstance* emitterInstance) override;
    void RenderEditorInspector() override;

  private:
    float currentEmissionTime                      = 0.f;
    float duration                                 = 5.f;
    bool loop                                      = false;
    int maxParticles                               = 100;

    bool randomLifetime                            = false;
    float minLifetime                              = 0.5f;
    float maxLifetime                              = 3.f;
};
