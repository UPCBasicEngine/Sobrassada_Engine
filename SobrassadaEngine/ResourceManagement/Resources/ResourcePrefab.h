#pragma once

#include "Resource.h"

#include <unordered_map>
#include <vector>

class GameObject;

class ResourcePrefab : public Resource
{
  public:
    ResourcePrefab(UID uid, UID versionUid, const std::string& name);
    ~ResourcePrefab() override;

    void LoadData(const std::vector<GameObject*>& objects, const std::vector<int>& indices);

    GameObject* GetRootObject() const { return gameObjects[0]; }
    void GetGameObjectsMap(std::unordered_map<UID, GameObject*>& mapToFill);
    const std::vector<GameObject*>& GetGameObjectsVector() const { return gameObjects; }
    const std::vector<int>& GetParentIndices() const { return parentIndices; }
    const UID GetVersionUID() const { return versionUID; }

  private:
    UID versionUID = INVALID_UID;
    std::vector<GameObject*> gameObjects;
    std::vector<int> parentIndices;
};
