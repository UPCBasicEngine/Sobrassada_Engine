#pragma once

#include "Globals.h"

#include <cstdint>
#include <string>

// order matters
enum class ResourceType
{
    Unknown = 10,
    Mesh,      // 11
    Texture,   // 12
    Material,  // 13
    Model,     // 14
    Animation, // 15
    Prefab,    // 16
    StateMachine,//17
    Navmesh,   // 18
    Font,         // 19
    ParticleEmmiter,
    ParticleSystem,
};

class SOBRASADA_API_ENGINE Resource
{
  public:
    Resource(UID uid, const std::string& name, ResourceType type);
    virtual ~Resource();

    void AddReference() { referenceCount++; }
    void RemoveReference() { referenceCount--; }

    UID GetUID() const { return uid; }
    const std::string& GetName() const { return name; }
    void SetName(std::string newName) { name = newName; }
    ResourceType GetType() const { return type; }
    unsigned int GetReferenceCount() const { return referenceCount; }
    static ResourceType GetResourceTypeForUID(UID uid) { return static_cast<ResourceType>(uid / UID_PREFIX_DIVISOR); }

  protected:
    UID uid = INVALID_UID;
    std::string name;
    ResourceType type           = ResourceType::Unknown;
    unsigned int referenceCount = 0;
};
