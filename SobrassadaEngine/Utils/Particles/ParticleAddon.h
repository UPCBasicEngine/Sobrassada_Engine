#pragma once

#include "ParticleUtils.h"

#include "rapidjson/document.h"

class EmitterInstance;

class ParticleAddon
{
  public:
    ParticleAddon(ParticleAddonType type) : addonType(type) {};
    ParticleAddon(const rapidjson::Value& initialState);
    virtual ~ParticleAddon() = default;

    virtual void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const;

    virtual void Init(EmitterInstance* emitterInstance) {};
    virtual void Update(float deltaTime, EmitterInstance* emitterInstance) = 0;
    virtual void RenderEditorInspector() = 0;

    ParticleAddonType GetType() const { return addonType; };
    bool IsEnabled() const { return isEnabled; }

    void Enable() { isEnabled = true; }
    void Disable() { isEnabled = false; }

  protected:
    bool isEnabled                = true;
    ParticleAddonType addonType   = ParticleAddonType::NONE;
};
