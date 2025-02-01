#include "Component.h"

#include "Application.h"
#include "ComponentUtils.h"
#include "SceneModule.h"

#include <Algorithm/Random/LCG.h>

Component::Component(const uint32_t uuid, const uint32_t ownerUUID, const char* name): uuid(uuid), ownerUUID(ownerUUID), name(name), enabled(true){}

void Component::Enable()
{
    enabled = true;
}

void Component::Disable()
{
    enabled = false;
}

bool Component::CreateComponent(const ComponentType componentType)
{
    // TODO Call library to create the component with an id instead
    Component* createdComponent = ComponentUtils::CreateEmptyComponent(componentType, LCG().IntFast(), uuid);
    
    if (createdComponent != nullptr) {
        App->GetSceneModule()->gameComponents[createdComponent->GetUUID()] = createdComponent;
        children.push_back(createdComponent->GetUUID());
        return true;
    }
    return false;
}

bool Component::AddComponent(const uint32_t componentUUID)
{
    children.push_back(componentUUID);
    return true;
}

bool Component::RemoveComponent(const uint32_t componentUUID)
{
    if (const auto it = std::find(children.begin(), children.end(), componentUUID); it != children.end())
    {
        children.erase(it);
        return true;
    }
    return false;
}

void Component::RenderEditorInspector()
{
}

void Component::RenderEditorComponentTree()
{
}

uint32_t Component::GetUUID() const
{
    return uuid;
}