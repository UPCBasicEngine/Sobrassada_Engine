#pragma once
#include "Globals.h"


#include <cstdint>
#include <string>

enum class ResourceType
{
    Unknown = 0,
    Texture,
    Mesh,
    Audio,
    Scene,
};

class Resource
{
public:
    
    Resource (UID uid, ResourceType type);

    virtual ~Resource();

    const char* GetAssetsFile() const;
    const char* GetLibraryFile() const;
    
    UID GetUID() const { return uid; }
    const std::string& GetName() const { return assetName; }
    ResourceType GetType() const { return type; }
    void AddReference() { referenceCount++; }
    void RemoveReference() { referenceCount--; }
    unsigned int GetReferenceCount() const { return referenceCount; }

protected:

    UID uid = CONSTANT_EMPTY_UID;
    std::string assetName;
    std::string assetsFile;
    std::string libraryFile;

    ResourceType type = ResourceType::Unknown;
    unsigned int referenceCount = 0;
};
