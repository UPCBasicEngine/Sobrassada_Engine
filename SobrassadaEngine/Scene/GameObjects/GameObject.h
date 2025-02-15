#pragma once
#include "Globals.h"
#include "Scene/AABBUpdatable.h"
#include "Transform.h"

#include <string>
#include <vector>
#include <Geometry/AABB.h>

class RootComponent;

class GameObject : public AABBUpdatable
{
public:

    GameObject(std::string name);
    GameObject(UID parentUUID, std::string name);
    GameObject(UID parentUUID, std::string name, UID rootComponent);

    ~GameObject();

	bool AddGameObject(UID gameObjectUUID);
    bool RemoveGameObject(UID gameObjectUUID);

    bool CreateRootComponent();

    void OnEditor();

    void SaveToLibrary();

    void RenderHierarchyNode(UID &selectedGameObjectUUID);
    void HandleNodeClick(UID &selectedGameObjectUUID);
    void RenderContextMenu();

    bool UpdateGameObjectHierarchy(UID sourceUID, UID targetUID);

	inline std::string GetName() const { return name; }
    void SetName(std::string newName) { name = newName; }
    
	inline std::vector<UID> GetChildren() { return children; }
    inline void AddChildren(UID childUUID) { children.push_back(childUUID); }
    
	inline UID GetParent() const { return parentUUID; }
    void SetParent(UID newParentUUID) { parentUUID = newParentUUID; }

    inline UID GetUID() const { return uuid; }
	void SetUUID(UID newUUID) { uuid = newUUID; }
    
    RootComponent* GetRootComponent() const { return rootComponent; }

    inline const AABB &GetAABB() const { return globalAABB; };
    
    void Render();
    void RenderEditor();

    void PassAABBUpdateToParent() override;
    void ComponentGlobalTransformUpdated() override;
    const Transform& GetGlobalTransform() const override;

private:

	UID parentUUID;
	UID uuid;
	std::vector<UID> children;

    std::string name;

    RootComponent *rootComponent;

    AABB globalAABB;

};
