#pragma once

#include "Globals.h"
#include "Particle.h"
#include "ParticleUtils.h"

#include "rapidjson/document.h"
#include <bitset>
#include <string>
#include <tuple>
#include <vector>

namespace math
{
    class float4x4;
}

class BaseAddon;
class VelocityAddon;
class ResourceTexture;
class ResourceMaterial;
class ParticleSystemComponent;
class EmitterInstance;

class ParticleEmitter
{
  public:
    ParticleEmitter(UID uid, const std::string& name, ParticleSystemComponent* owner);
    ParticleEmitter(const rapidjson::Value& initialState, ParticleSystemComponent* owner);
    ~ParticleEmitter();

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const;

    void Update(float deltaTime, EmitterInstance* emitterInstance);
    void Spawn();
    
    // TEMPORAL, PROBABLY CAN SEND ACTIVES PARTICLES TO WHOLE BATCH OF EMITTERS THAT SHARE SAME TEXTURE
    void RenderParticles(const float4x4& VP, const float3& rightVector, const float3& upVector);
    
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

    // TEMPORAL, PROBABLY CAN SEND ACTIVES PARTICLES TO WHOLE BATCH OF EMITTERS THAT SHARE SAME TEXTURE
    void UpdateParticlesVBO();

  public:
    bool isEmitting = false;
    std::vector<Particle> particles;

  private:
    unsigned int quadVBO               = 0;
    // TEMPORAL, PROBABLY CAN SEND ACTIVES PARTICLES TO WHOLE BATCH OF EMITTERS THAT SHARE SAME TEXTURE
    unsigned int particlesVBO          = 0;

    unsigned int aliveParticles        = 0;

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
