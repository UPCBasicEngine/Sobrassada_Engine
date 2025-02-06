#include "GameObject.h"

GameObject::GameObject(std::string name) : name(name) {}

GameObject::GameObject(uint32_t parentUUID, std::string name) : parentUUID(parentUUID), name(name) {}

bool GameObject::AddGameObject(uint32_t gameObjectUUID)
{
    if (std::find(children.begin(), children.end(), gameObjectUUID) == children.end())
    {
        children.push_back(gameObjectUUID);
        return true; 
    }
    return false;
}

bool GameObject::RemoveGameObject(uint32_t gameObjectUUID)
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

void GameObject::SetParent(uint32_t newParentUUID)
{ 
    parentUUID = newParentUUID; 
}