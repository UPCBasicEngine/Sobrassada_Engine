#include "MeshComponent.h"

MeshComponent::MeshComponent(const uint32_t uuid, const uint32_t uuidParent, const uint32_t uuidRoot, const char* name, const Transform& parentGlobalTransform)
        : Component(uuid, uuidParent, uuidRoot, name, parentGlobalTransform)
{
}

void MeshComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();
}

void MeshComponent::RenderEditorComponentTree(const uint32_t selectedComponentUUID)
{
    Component::RenderEditorComponentTree(selectedComponentUUID);
}

void MeshComponent::Update()
{
}