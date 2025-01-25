#include "ModelComponent.h"

ModelComponent::ModelComponent(const uint32_t uuid, const uint32_t ownerUUID, const char* name)
        : Component(uuid, ownerUUID, name)
{
}

void ModelComponent::RenderEditorInspector()
{
}

void ModelComponent::RenderEditorComponentTree()
{
}

void ModelComponent::Update()
{
}

void ModelComponent::LoadModel(uint32_t modelUUID)
{
    currentModelUUID = modelUUID;
}