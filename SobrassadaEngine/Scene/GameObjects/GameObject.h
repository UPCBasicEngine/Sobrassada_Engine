#pragma once
#include "Globals.h"
#include "BoundingBox.h"
#include "Transform.h"
#include "Scene/AABBUpdatable.h"

#include <vector>
#include <string>

class RootComponent;

class GameObject : public AABBUpdatable
{
public:

	GameObject(std::string name);
	GameObject(UID parentUUID, std::string name);

    ~GameObject();

	bool AddGameObject(UID gameObjectUUID);
    bool RemoveGameObject(UID gameObjectUUID);

    bool CreateRootComponent();

	void OnEditor();

	void SaveToLibrary();

	inline std::string GetName() { return name; }
    void SetName(std::string newName) { name = newName; }
    
	inline std::vector<UID> GetChildren() { return children; }
    
	inline UID GetParent() { return parentUUID; }
    void SetParent(UID newParentUUID) { parentUUID = newParentUUID; }

	void SetUUID(UID newUUID) { uuid = newUUID; }
    
    RootComponent* GetRootComponent() const { return rootComponent; }
    
    void Render();
    void RenderEditor();
    
    void PassAABBUpdateToParent() override;
    const Transform& GetGlobalTransform() const override;

private:

	UID parentUUID;
	UID uuid;
	std::vector<UID> children;

	std::string name;

        RootComponent *rootComponent;

	BoundingBox boundingBox;
};
