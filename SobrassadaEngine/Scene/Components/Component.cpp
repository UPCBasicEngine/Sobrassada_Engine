#include "Component.h"

Component::Component(char *ownerUUID, char* name): ownerUUID(ownerUUID), name(name), enabled(true){}

void Component::Enable()
{
    enabled = true;
}

void Component::Update()
{
}

void Component::Disable()
{
    enabled = false;
}

bool Component::AddComponent(const char *componentUUID)
{
    children.push_back(componentUUID);
    return true;
}

bool Component::RemoveComponent(const char *componentUUID)
{
    if (const auto it = std::find(children.begin(), children.end(), componentUUID); it != children.end())
    {
        children.erase(it);
        return true;
    }
    return false;
}

void Component::RenderEditor()
{
}