#include "Importer.h"

#include "Application.h"
#include "LibraryModule.h"
#include "MaterialImporter.h"
#include "MeshImporter.h"
#include "ModelImporter.h"
#include "AnimationImporter.h"
#include "PrefabManager.h"
#include "FontImporter.h"
#include "StateMachineManager.h"
#include "Resource.h"
#include "ResourceMaterial.h"
#include "ResourceMesh.h"
#include "ResourceModel.h"
#include "ResourcePrefab.h"
#include "ResourceTexture.h"
#include "ResourceFont.h"
#include "ResourceStateMachine.h"
#include "TextureImporter.h"
#include "ResourceAnimation.h"

Resource* Importer::Load(UID uid)
{
    const ResourceType type = Resource::GetResourceTypeForUID(uid);

    switch (type)
    {
    case ResourceType::Unknown:
        return nullptr;
    case ResourceType::Texture:
        return TextureImporter::LoadTexture(uid);
    case ResourceType::Material:
        return MaterialImporter::LoadMaterial(uid);
    case ResourceType::Mesh:
        return MeshImporter::LoadMesh(uid);
    case ResourceType::Model:
        return ModelImporter::LoadModel(uid);
    case ResourceType::Animation:
    return AnimationImporter::LoadAnimation(uid);
    case ResourceType::Prefab:
        return PrefabManager::LoadPrefab(uid);
    case ResourceType::StateMachine:
        return StateMachineManager::LoadStateMachine(uid);
    case ResourceType::Font:
        return FontImporter::LoadFont(uid);

    default:
        //GLOG("Unknown resource type: %d", type)
        return nullptr;
    }

    return nullptr;
}