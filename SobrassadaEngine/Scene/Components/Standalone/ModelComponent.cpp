#include "ModelComponent.h"

#include "Application.h"
#include "../Root/RootComponent.h"
#include "EditorUIModule.h"
#include "SceneModule.h"
#include "imgui.h"

#include <Algorithm/Random/LCG.h>

ModelComponent::ModelComponent(const uint32_t uuid, const uint32_t uuidParent, const uint32_t uuidRoot, const char* name)
        : Component(uuid, uuidParent, uuidRoot, name)
{
}

void ModelComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();
}

void ModelComponent::RenderEditorComponentTree(const uint32_t selectedComponentUUID)
{
    Component::RenderEditorComponentTree(selectedComponentUUID);
}

void ModelComponent::Update()
{
}

void ModelComponent::LoadModel(uint32_t modelUUID)
{
    currentModelUUID = modelUUID;
}