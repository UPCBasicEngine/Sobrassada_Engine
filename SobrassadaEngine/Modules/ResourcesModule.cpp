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
    if (resource != nullptr)
    {
        std::map<UID, Resource*>::iterator it = resources.find(resource->GetUID());

        if(it != resources.end())
        {
            it->second->RemoveReference();
        }
    }
}

Resource * ResourcesModule::CreateNewResource(const char *assetsFile, ResourceType type)
{
    MOCKUP_loadedModel = new EngineModel();
    MOCKUP_loadedModel->Load("./Test/BakerHouse.gltf");

    const uint32_t bakerHouseID = LCG().IntFast();
    const uint32_t bakerHouseChimneyID = LCG().IntFast();

    const uint32_t bakerHouseTextureID = LCG().IntFast();
    
    MOCKUP_loadedMeshes[bakerHouseID] = MOCKUP_loadedModel->GetMesh(1);
    MOCKUP_loadedMeshes[bakerHouseChimneyID] = MOCKUP_loadedModel->GetMesh(0);
    MOCKUP_libraryMeshes["Baker house"] = bakerHouseID;
    MOCKUP_libraryMeshes["Baker house chimney"] = bakerHouseChimneyID;

    // TODO Always have a default grid texture loaded to apply as standard
    MOCKUP_loadedTextures[bakerHouseTextureID] = MOCKUP_loadedModel->GetActiveRenderTexture();
    MOCKUP_libraryTextures["Baker house texture"] = bakerHouseTextureID;
    return nullptr;
}
