#pragma once

#include "Resource.h"

#include <unordered_map>

class GameObject;

class ResourcePrefab : public Resource
{
  public:
    ResourcePrefab(UID uid, const std::string& name);
    ~ResourcePrefab() override;

    void LoadData(UID rootUID, const std::vector<GameObject*>& objects);

    GameObject* GetRootObject() const { return gameObjectsContainer.at(gameObjectRootUID); }
    const std::unordered_map<UID, GameObject*>& GetGameObjectsContainer() const { return gameObjectsContainer; }

  private:

    UID gameObjectRootUID       = INVALID_UID;
    std::unordered_map<UID, GameObject*> gameObjectsContainer;
};
