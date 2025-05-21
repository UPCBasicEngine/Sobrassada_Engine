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
    void RenderParticles();
    void RenderEditor();

    void AddAddon(ParticleAddonType type);
    void RemoveAddon(ParticleAddonType type);

    UID GetUID() const { return uid; }
    const std::string& GetName() const { return name; }
    std::tuple<ADDON_TYPES>& GetAddonsTuple() { return addonTuple; }
    ParticleSystemComponent* GetOwner() { return owner; }

    void SetAddonCreated(int position) { createdAddons[position] = true; };
    void SetAddonDeleted(int position) { createdAddons[position] = false; };
    void SetQuadVBO(unsigned int newVbo) { quadVBO = newVbo; };

  private:
    void UpdateMaterial(UID newMaterialUID);
    void UpdateTexture(UID newTextureUID);

  public:
    bool spawning = false;
    std::vector<Particle> particles;

  private:
    unsigned int quadVBO               = 0;
    unsigned int particlesVBO          = 0;

    UID uid                            = INVALID_UID;
    std::string name                   = "";

    bool useTexture                    = false;
    std::string currentResourceName    = "No material";

    ResourceTexture* texture           = nullptr;
    ResourceMaterial* material         = nullptr;
    ParticleSystemComponent* owner     = nullptr;

    std::tuple<ADDON_TYPES> addonTuple = std::make_tuple(ADDON_NULLPTR);
    std::bitset<std::tuple_size<decltype(addonTuple)>::value> createdAddons;
};
