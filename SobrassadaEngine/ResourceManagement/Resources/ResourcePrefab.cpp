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

void ResourcePrefab::LoadData(const std::unordered_map<UID, GameObject*>& objects)
{
    gameObjectsContainer = objects;
}
