#include "Importer.h"

#include "Application.h"
#include "LibraryModule.h"
#include "MeshImporter.h"

ResourceMesh* Importer::Load(UID uid)
{
    const std::string path = App->GetLibraryModule()->GetResourcePath(uid);
    const ResourceType type = Resource::GetResourceTypeForUID(uid);

    switch (type) {
        case ResourceType::Unknown:
            return nullptr;
        case ResourceType::Texture:
            break;  // TODO Material loadTexture()
        case ResourceType::Material:
            break;  // TODO Material loadMaterial()
        case ResourceType::Mesh:
            return MeshImporter::LoadMesh(path);
        default:
            GLOG("Unknown resource type: %d", type)
            return nullptr;
    }
    return nullptr;
}