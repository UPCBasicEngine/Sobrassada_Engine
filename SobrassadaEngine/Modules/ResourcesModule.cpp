#include "ResourcesModule.h"

#include "Application.h"
#include "BatchManager.h"
#include "GameObject.h"
#include "Importer.h"
#include "LibraryModule.h"
#include "MeshImporter.h"
#include "PathfinderModule.h"
#include "Resource.h"
#include "ResourceMaterial.h"
#include "ResourceMesh.h"
#include "ResourceNavmesh.h"
#include "ResourceTexture.h"
#include "SceneModule.h"
#include "ShaderModule.h"


ResourcesModule::ResourcesModule()
{
    batchManager = new BatchManager();
}

ResourcesModule::~ResourcesModule()
{
    delete batchManager;
}


bool ResourcesModule::ShutDown()
{
    UnloadAllResources();
    batchManager->UnloadAllBatches();
    return true;
}

Resource* ResourcesModule::RequestResource(UID uid)
{
    // Find if the resource is already loaded
    std::map<UID, Resource*>::iterator it = resources.find(uid);

    if (it != resources.end())
    {
        it->second->AddReference();
        return it->second;
    }

    // Find the library file (if exists) and load the custom file format
    return CreateNewResource(uid);
}

void ResourcesModule::ReleaseResource(const Resource* resource)
{
    if (resource != nullptr)
        ReleaseResource(resource->GetUID());
}

void ResourcesModule::ReleaseResource(UID resourceUID)
{
    std::map<UID, Resource*>::iterator it = resources.find(resourceUID);

    if (it != resources.end())
    {
        it->second->RemoveReference();
        if (it->second->GetReferenceCount() <= 0)
        {
            delete it->second;
            resources.erase(it);
        }
    }
}

Resource* ResourcesModule::CreateNewResource(UID uid)
{
    Resource* loadedResource = Importer::Load(uid);
    if (loadedResource != nullptr)
    {
        resources.insert(std::pair(uid, loadedResource));
        loadedResource->AddReference();
        return loadedResource;
    }
    return nullptr;
}

void ResourcesModule::UnloadAllResources()
{
    for (auto resource : resources)
    {
        delete resource.second;
    }
    resources.clear();
}

