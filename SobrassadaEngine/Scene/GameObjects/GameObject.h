#pragma once
#include "BoundingBox.h"
#include "Root/RootComponent.h"

#include <vector>
#include <string>

class GameObject
{
public:

	GameObject(std::string name);
	GameObject(uint32_t parentUUID, std::string name);

    ~GameObject();

	bool AddGameObject(uint32_t gameObjectUUID);
    bool RemoveGameObject(uint32_t gameObjectUUID);

    bool CreateRootComponent();

	void OnEditor();

	void SaveToLibrary();

	inline std::string GetName() { return name; }
    inline void SetName(std::string newName) { name = newName; }
    
	inline std::vector<uint32_t> GetChildren() { return children; }
    
	inline uint32_t GetParent() { return parentUUID; }
    inline void SetParent(uint32_t newParentUUID) { parentUUID = newParentUUID; }
    
    RootComponent* GetRootComponent() const { return rootComponent; }
    
    void Render();
    void RenderEditor();

private:

	uint32_t parentUUID;
	std::vector<uint32_t> children;

	std::string name;

        RootComponent *rootComponent;

	BoundingBox boundingBox;
};
