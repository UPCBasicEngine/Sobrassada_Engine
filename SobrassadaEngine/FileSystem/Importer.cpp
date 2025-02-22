#include "Importer.h"

#include "Application.h"
#include "LibraryModule.h"
#include "MeshImporter.h"
#include "TextureImporter.h"
#include "MaterialImporter.h"
#include "AnimationImporter.h"

Resource* Importer::Load(UID uid)
{
    const ResourceType type = Resource::GetResourceTypeForUID(uid);
    ResourceAnimation* anim;

    switch (type) {
        case ResourceType::Unknown:
            return nullptr;
        case ResourceType::Texture:
            return TextureImporter::LoadTexture(uid);
        case ResourceType::Material:
            return MaterialImporter::LoadMaterial(uid);
        case ResourceType::Mesh:
            
            return MeshImporter::LoadMesh(uid);
        case ResourceType::Animation:
            GLOG("HOLA");
            return AnimationImporter::LoadAnimation(uid);
        default:
            GLOG("Unknown resource type: %d", type)
            return nullptr;
    }

    return nullptr;
}