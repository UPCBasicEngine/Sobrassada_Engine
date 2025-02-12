#pragma once

#include "EngineModel.h"
#include "Module.h"
#include "ResourceManagement/Resources/Resource.h"

#include <map>

class ResourcesModule : public Module
{
public:

    ResourcesModule();
    ~ResourcesModule() override;

    bool Init() override;
   
    bool ShutDown() override;

    Resource* RequestResource(UID uid);
    void ReleaseResource(Resource* resource);

    int GetProgram() const { return program; }

    std::map<std::string, UID> MOCKUP_libraryMeshes;   // TODO Move to library file
    std::map<std::string, UID> MOCKUP_libraryTextures; // TODO Move to library file

private:
    int program = -1;
    
    Resource* CreateNewResource(const char* assetsFile, ResourceType type);

    std::map<UID, Resource*> resources;
    
};
