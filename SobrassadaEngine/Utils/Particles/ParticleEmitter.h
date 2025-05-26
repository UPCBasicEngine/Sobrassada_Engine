#pragma once

#include "Globals.h"
#include "HashString.h"
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
class SpritesheetAddon;
class ColorAddon;

class ParticleSystem;
class ResourceTexture;
class ResourceMaterial;
class ParticleSystemComponent;
class EmitterInstance;

class ParticleEmitter
{
  public:
    ParticleEmitter(const HashString& tag, ParticleSystem* owner);
    ParticleEmitter(const rapidjson::Value& initialState, ParticleSystem* owner);
    ~ParticleEmitter();

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const;

    void Update(float deltaTime, EmitterInstance* emitterInstance);
    void Spawn(EmitterInstance* emitterInstance);

    // TEMPORAL, PROBABLY CAN SEND ACTIVES PARTICLES TO WHOLE BATCH OF EMITTERS THAT SHARE SAME TEXTURE
    void RenderParticles(const float4x4& VP, const float3& rightVector, const float3& upVector);

    void RenderEditor();

    void AddAddon(ParticleAddonType type);
    void RemoveAddon(ParticleAddonType type);

    const HashString& GetTag() const { return emitterTag; }
    const std::string& GetName() const { return emitterTag.GetString(); }
    std::tuple<ADDON_TYPES>& GetAddonsTuple() { return addonTuple; }
    int GetRenderPriority() const { return renderPriority; }

    void SetAddonCreated(int position) { createdAddons[position] = true; };
    void SetAddonDeleted(int position) { createdAddons[position] = false; };
    void SetQuadVBO(unsigned int newVbo) { quadVBO = newVbo; };
    void SetUseSpritesheet(bool spritesheet) { useSpritesheet = spritesheet; };

  private:
    void UpdateMaterial(UID newMaterialUID);
    void UpdateTexture(UID newTextureUID);
    void UpdateParticlesVBO(EmitterInstance* emitterInstance);

  private:
    unsigned int quadVBO               = 0;
    unsigned int particlesVBO          = 0;
    unsigned int particleTileOffsetVBO = 0;
    unsigned int particleColorsVBO     = 0;

    int renderPriority                 = 0;

    bool useSpritesheet                = false;

    HashString emitterTag              = HashString("");

    bool useTexture                    = false;

    ResourceTexture* texture           = nullptr;
    ResourceMaterial* material         = nullptr;

    ParticleSystem* owner              = nullptr;

    std::vector<float3> alivePositions;
    std::vector<std::pair<int, int>> tileOffsets;
    std::vector<float4> particleColors;

    std::vector<Particle> batchedParticles;

    std::tuple<ADDON_TYPES> addonTuple = std::make_tuple(ADDON_NULLPTR);
    std::bitset<std::tuple_size<decltype(addonTuple)>::value> createdAddons;
};
