#pragma once

#include "Globals.h"
#include "Particle.h"
#include "ParticleUtils.h"

#include "rapidjson/document.h"
#include <bitset>
#include <string>
#include <tuple>
#include <vector>

class BaseAddon;
class VelocityAddon;
class ResourceTexture;
class ResourceMaterial;
class ParticleSystemComponent;

class ParticleEmitter
{
  public:
    ParticleEmitter(UID uid, const std::string& name, ParticleSystemComponent* owner);
    ParticleEmitter(const rapidjson::Value& initialState, ParticleSystemComponent* owner);
    ~ParticleEmitter();

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const;

    void Update(float deltaTime);
    void Spawn();
    void RenderEditor();

    void AddAddon(ParticleAddonType type);
    void RemoveAddon(ParticleAddonType type);

    UID GetUID() const { return uid; }
    const std::string& GetName() const { return name; }
    std::tuple<ADDON_TYPES>& GetAddonsTuple() { return addonTuple; }

    void SetAddonCreated(int position) { createdAddons[position] = true; };
    void SetAddonDeleted(int position) { createdAddons[position] = false; };

    ParticleSystemComponent* GetOwner() { return owner; }
    void SetOwner(ParticleSystemComponent* newOwner) { owner = newOwner; };

  public:
    std::vector<Particle> particles;

  private:
    UID uid                            = INVALID_UID;
    std::string name                   = "";
    bool useTexture                    = false;
    ResourceTexture* texture           = nullptr;
    ResourceMaterial* material         = nullptr;
    ParticleSystemComponent* owner     = nullptr;

    std::tuple<ADDON_TYPES> addonTuple = std::make_tuple(ADDON_NULLPTR);
    std::bitset<std::tuple_size<decltype(addonTuple)>::value> createdAddons;
};
