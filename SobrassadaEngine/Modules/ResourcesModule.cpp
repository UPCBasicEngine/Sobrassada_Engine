#include "ResourcesModule.h"

#include "Application.h"
#include "ShaderModule.h"

#include <Algorithm/Random/LCG.h>

ResourcesModule::ResourcesModule()
{
}

ResourcesModule::~ResourcesModule()
{
    
}

bool ResourcesModule::Init()
{
    //CreateNewResource("", ResourceType::Mesh);
    program = App->GetShaderModule()->GetProgram("./Test/VertexShader.glsl", "./Test/FragmentShader.glsl");
    
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
    
    EngineModel* MOCKUP_loadedModel = new EngineModel();
    MOCKUP_loadedModel->Load("./Test/BakerHouse.gltf");

    const uint32_t bakerHouseID = LCG().IntFast();
    const uint32_t bakerHouseChimneyID = LCG().IntFast();
    
    resources[bakerHouseID] = MOCKUP_loadedModel->GetMesh(1);
    resources[bakerHouseChimneyID] = MOCKUP_loadedModel->GetMesh(0);

    return nullptr;
}
