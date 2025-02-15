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
    void ReleaseResource(const Resource* resource);

    int GetProgram() const { return program; }

private:
    int program = -1;
    
    Resource* CreateNewResource(UID uid);

    std::map<UID, Resource*> resources;
    
};
