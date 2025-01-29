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

private:

	uint32_t parentUUID;
	std::vector<uint32_t> children;

	std::string name;

	uint32_t rootComponentUUID;

	BoundingBox boundingBox;
};
