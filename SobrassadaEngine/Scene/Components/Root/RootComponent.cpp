#include "RootComponent.h"

RootComponent::RootComponent(const uint32_t uuid, const uint32_t ownerUUID, const char* name)
        : Component(uuid, ownerUUID, name)
{
}

bool RootComponent::AddComponent(const uint32_t componentUUID)
{
    // TODO Load component from storage
    // TODO Make sure passed componentUUID encodes a standalone component
    return Component::AddComponent(componentUUID);
}

bool RootComponent::RemoveComponent(const uint32_t componentUUID)
{
    return Component::RemoveComponent(componentUUID);
}

void RootComponent::RenderEditorInspector()
{
}

void RootComponent::Update()
{
}