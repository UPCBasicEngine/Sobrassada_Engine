#include "Importer.h"

#include "Application.h"
#include "LibraryModule.h"
#include "MeshImporter.h"
#include "TextureImporter.h"
#include "ResourceManagement/Resources/ResourceTexture.h"

Resource* Importer::Load(UID uid)
{
    const ResourceType type = Resource::GetResourceTypeForUID(uid);

    switch (type) {
        case ResourceType::Unknown:
            return nullptr;
        case ResourceType::Texture:
            return TextureImporter::LoadTexture(uid);
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