#include "ResourcePrefab.h"
#include "GameObject.h"

ResourcePrefab::ResourcePrefab(UID uid, const std::string& name) : Resource(uid, name, ResourceType::Prefab)
{
}

ResourcePrefab::~ResourcePrefab()
{
    for (auto& object : gameObjects)
    {
        delete object;
    }
    gameObjects.clear();
    parentIndices.clear();
}

void ResourcePrefab::LoadData(const std::vector<GameObject*>& objects, const std::vector<int>& indices)
{
    gameObjects = objects;
    parentIndices = indices;
}