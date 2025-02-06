#pragma once
#include "BoundingBox.h"

#include <vector>
#include <string>

class GameObject
{
public:

	GameObject(std::string name);
	GameObject(uint32_t parentUUID, std::string name);

	bool AddGameObject(uint32_t gameObjectUUID);
    bool RemoveGameObject(uint32_t gameObjectUUID);

	void OnEditor();

	void SaveToLibrary();

	std::string GetName() { return name; }
    std::vector<uint32_t> GetChildren() { return children; }
    
	uint32_t GetParent() { return parentUUID; }
	void SetParent(uint32_t newParentUUID);

private:

	uint32_t parentUUID;
	std::vector<uint32_t> children;

	std::string name;

	uint32_t rootComponentUUID;

	BoundingBox boundingBox;
};
