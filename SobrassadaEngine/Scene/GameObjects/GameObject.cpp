#include "GameObject.h"

GameObject::GameObject(char *parentUUID, char *name) : parentUUID(parentUUID), name(name) {}

bool GameObject::AddGameObject(const char* gameObjectUUID)
{
    children.push_back(gameObjectUUID);
	return true; 
}

bool GameObject::RemoveGameObject(const char* gameObjectUUID)
{
    if (const auto it = std::find(children.begin(), children.end(), gameObjectUUID); it != children.end())
    {
        children.erase(it);
        return true;
    }
    return false;
}

void GameObject::OnEditor()
{
}

void GameObject::SaveToLibrary()
{
}