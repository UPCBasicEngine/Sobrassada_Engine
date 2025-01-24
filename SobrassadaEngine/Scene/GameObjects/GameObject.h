#pragma once
#include "BoundingBox.h"

#include <vector>

class GameObject
{
public:

	GameObject(char* parentUUID, char* name);

	bool AddGameObject(const char* gameObjectUUID);
	bool RemoveGameObject(const char *gameObjectUUID);

	void OnEditor();

	void SaveToLibrary();

private:

	char* parentUUID;
	std::vector<const char *> children;

	char* name;

	char* rootComponentUUID;

	BoundingBox boundingBox;
};
