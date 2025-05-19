#pragma once

#include "rapidjson/document.h"

enum class ParticleAddonType : int
{
    BASE = 0,

};

class ResourceEmitter;

class ParticleAddon
{
  public:
    ParticleAddon() = default;
    virtual ~ParticleAddon() = default;

    virtual void Save(const rapidjson::Value& initialState);

    virtual void Update(float deltaTime);
    virtual void RenderEditor();

    bool IsEnabled() const { return IsEnabled; }

    void Enable() { isEnabled = true; }
    void Disable() { isEnabled = false; }

  private:
    bool isEnabled = false;
    ResourceEmitter* emitterOwner = nullptr;
    ParticleAddonType addonType   = ParticleAddonType::BASE;
};
