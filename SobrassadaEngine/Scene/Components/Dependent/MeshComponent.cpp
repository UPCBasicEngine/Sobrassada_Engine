#include "MeshComponent.h"

MeshComponent::MeshComponent(const uint32_t uuid, const uint32_t uuidParent, const uint32_t uuidRoot, const char* name)
        : Component(uuid, uuidParent, uuidRoot, name)
{
}

void MeshComponent::RenderEditorInspector()
{
}

void MeshComponent::RenderEditorComponentTree(const uint32_t selectedComponentUUID)
{
}

void MeshComponent::Update()
{
}