#include "Importer.h"

#include "Application.h"
#include "LibraryModule.h"
#include "MeshImporter.h"

//change to enum
ResourceMesh* Importer::Load(UID uid)
{
    //this cannot happen before it's loaded into the resources
    
    const ResourceType type = Resource::GetResourceTypeForUID(uid);

    switch (type) {
        case ResourceType::Unknown:
            return nullptr;
        case ResourceType::Texture:
            break;  // TODO Material loadTexture()
        case ResourceType::Material:
            break;  // TODO Material loadMaterial()
        case ResourceType::Mesh:
            return MeshImporter::LoadMesh(uid);
            
        default:
            GLOG("Unknown resource type: %d", type)
            return nullptr;
    }

    return nullptr;
}