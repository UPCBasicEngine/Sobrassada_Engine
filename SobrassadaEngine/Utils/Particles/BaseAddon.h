#pragma once

#include "ParticleAddon.h"

class BaseAddon : public ParticleAddon
{
  public:
    BaseAddon(ParticleEmitter* owner);
    BaseAddon(const rapidjson::Value& initialState, ParticleEmitter* owner);
    ~BaseAddon() override;

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;

    void Init() const;
    void Update(float deltaTime) const override;
    void RenderEditorInspector() override;

  private:
    float duration = 5.f;
    bool loop = false;
    unsigned int maxParticles = 100;
};
