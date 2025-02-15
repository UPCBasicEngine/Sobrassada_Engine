#pragma once
#include "Globals.h"


#include <cstdint>
#include <string>

enum class ResourceType
{
    Unknown = 10,
    Texture,
    Material,
    Mesh,
};

class Resource
{
public:
    
    Resource (UID uid, const std::string& name, ResourceType type);

    virtual ~Resource();
    
    UID GetUID() const { return uid; }
    const std::string& GetName() const { return name; }
    ResourceType GetType() const { return type; }
    void AddReference() { referenceCount++; }
    void RemoveReference() { referenceCount--; }
    unsigned int GetReferenceCount() const { return referenceCount; }

    static ResourceType GetResourceTypeForUID(UID uid)
    {
        return static_cast<ResourceType>(uid / 100000000000000);
    }

protected:

    UID uid = CONSTANT_EMPTY_UID;
    const std::string name;

    ResourceType type = ResourceType::Unknown;
    unsigned int referenceCount = 0;
};
