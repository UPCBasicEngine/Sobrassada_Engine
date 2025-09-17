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

void ResourcePrefab::LoadData(const std::vector<GameObject*>& objects)
{
    for (auto gameObject : objects)
    {
        gameObjectsContainer[gameObject->GetUID()] = gameObject;
    }
}
