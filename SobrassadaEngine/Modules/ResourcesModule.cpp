#include "ResourcesModule.h"

ResourcesModule::ResourcesModule()
{
}

ResourcesModule::~ResourcesModule()
{
    
}

bool ResourcesModule::Init()
{
    return true;
}

bool ResourcesModule::ShutDown()
{
    return true;
}

Resource * ResourcesModule::RequestResource(UID uid)
{
    //Find if the resource is already loaded	  

    std::map<UID, Resource*>::iterator it = resources.find(uid);

    if(it != resources.end())
    {
            it->second->AddReference();

            return it->second;
    }
    
//Find the library file (if exists) and load the custom file format

    // TODO Connect with file system return TryToLoadResource(uid);
    return nullptr;
}

void ResourcesModule::ReleaseResource(Resource *resource)
{
    std::map<UID, Resource*>::iterator it = resources.find(resource->GetUID());

    if(it != resources.end())
    {
        it->second->RemoveReference();
    }
}

Resource * ResourcesModule::CreateNewResource(const char *assetsFile, ResourceType type)
{
    return nullptr;
}
