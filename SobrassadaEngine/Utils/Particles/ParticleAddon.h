#pragma once

#include "rapidjson/document.h"

enum class ParticleAddonType : int
{
    NONE = 0,
    BASE,
    VELOCITY,
};

class ResourceEmitter;

class ParticleAddon
{
  public:
    ParticleAddon(ResourceEmitter* owner) : emitterOwner(owner) {};
    ParticleAddon(const rapidjson::Value& initialState, ResourceEmitter* owner);
    virtual ~ParticleAddon() = default;

    virtual void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const;

    virtual void Update(float deltaTime) = 0;
    virtual void RenderEditorInspector() = 0;

    bool IsEnabled() const { return IsEnabled; }

    void Enable() { isEnabled = true; }
    void Disable() { isEnabled = false; }

  private:
    bool isEnabled = false;
    ResourceEmitter* emitterOwner = nullptr;
    ParticleAddonType addonType   = ParticleAddonType::NONE;
};
