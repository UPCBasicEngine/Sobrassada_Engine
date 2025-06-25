#pragma once

#include "ParticleUtils.h"

#include "rapidjson/document.h"

class EmitterInstance;
class ParticleEmitter;
class GameObject;

class ParticleAddon
{
  public:
    ParticleAddon(ParticleAddonType type, ParticleEmitter* owner) : addonType(type), owner(owner) {};
    ParticleAddon(const rapidjson::Value& initialState, ParticleEmitter* owner);
    virtual ~ParticleAddon() = default;

    virtual void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const;

    virtual void Init(EmitterInstance* emitterInstance) {};
    virtual void Update(float deltaTime, EmitterInstance* emitterInstance) = 0;
    virtual void RenderEditorInspector()                                   = 0;
    virtual void RenderDebug(GameObject* parent) {};
    virtual void Duplicate(ParticleAddon* reference) = 0;

    ParticleAddonType GetType() const { return addonType; };
    bool IsEnabled() const { return isEnabled; }

    void Enable() { isEnabled = true; }
    void Disable() { isEnabled = false; }

  protected:
    bool isEnabled              = true;
    ParticleAddonType addonType = ParticleAddonType::NONE;
    ParticleEmitter* owner      = nullptr;
};
