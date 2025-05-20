#pragma once

#include "ParticleUtils.h"

#include "rapidjson/document.h"

class ParticleEmitter;

class ParticleAddon
{
  public:
    ParticleAddon(ParticleEmitter* owner, ParticleAddonType type) : emitterOwner(owner), addonType(type) {};
    ParticleAddon(const rapidjson::Value& initialState, ParticleEmitter* owner);
    virtual ~ParticleAddon() = default;

    virtual void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const;

    virtual void Init() const {};
    virtual void Update(float deltaTime) const = 0;
    virtual void RenderEditorInspector()       = 0;

    ParticleAddonType GetType() const { return addonType; };
    bool IsEnabled() const { return isEnabled; }

    void Enable() { isEnabled = true; }
    void Disable() { isEnabled = false; }

  private:
    bool isEnabled                = true;
    ParticleEmitter* emitterOwner = nullptr;
    ParticleAddonType addonType   = ParticleAddonType::NONE;
};
