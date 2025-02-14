#pragma once
#include "BoundingBox.h"
#include "Globals.h"
#include "Scene/AABBUpdatable.h"
#include "Transform.h"

#include <string>
#include <vector>

class RootComponent;

class GameObject : public AABBUpdatable
{
  public:
    GameObject(std::string name);
    GameObject(uint32_t parentUUID, std::string name);
    GameObject(UID parentUUID, std::string name, UID rootComponent);

    ~GameObject();

    bool AddGameObject(uint32_t gameObjectUUID);
    bool RemoveGameObject(uint32_t gameObjectUUID);

    bool CreateRootComponent();

    void OnEditor();

    void SaveToLibrary();

    inline std::string GetName() { return name; }
    void SetName(std::string newName) { name = newName; }

    inline const std::vector<uint32_t> &GetChildren() { return children; }
    inline void AddChildren(UID childUUID) { children.push_back(childUUID); }

    inline uint32_t GetParent() { return parentUUID; }
    void SetParent(uint32_t newParentUUID) { parentUUID = newParentUUID; }

    void SetUUID(uint32_t newUUID) { uuid = newUUID; }
    UID GetUUID() const { return uuid; }

    RootComponent *GetRootComponent() const { return rootComponent; }

    void Render();
    void RenderEditor();

    void PassAABBUpdateToParent() override;
    const Transform &GetGlobalTransform() const override;

  private:
    uint32_t parentUUID;
    uint32_t uuid;
    std::vector<uint32_t> children;

    std::string name;

    RootComponent *rootComponent;

    BoundingBox boundingBox;
};
