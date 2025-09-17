#include "ResourcePrefab.h"
#include "GameObject.h"

ResourcePrefab::ResourcePrefab(UID uid, const std::string& name) : Resource(uid, name, ResourceType::Prefab)
{
}

ResourcePrefab::~ResourcePrefab()
{
    for (auto& object : gameObjectsContainer)
    {
        delete object.second;
    }
    gameObjectsContainer.clear();
}

void ResourcePrefab::LoadData(UID rootUID, const std::vector<GameObject*>& objects)
{
    gameObjectRootUID = rootUID;
    for (auto gameObject : objects)
    {
        gameObjectsContainer[gameObject->GetUID()] = gameObject;
    }
}
