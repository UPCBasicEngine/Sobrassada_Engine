#include "ModelComponent.h"

#include <Algorithm/Random/LCG.h>

ModelComponent::ModelComponent(const uint32_t uuid, const uint32_t ownerUUID, const char* name)
        : Component(uuid, ownerUUID, name)
{
}

void ModelComponent::RenderEditorInspector()
{
    ImGui::Text("Local transform");
    localTransform.RenderEditor();
    
    ImGui::Spacing();
    
    ImGui::Text("Global transform");
    globalTransform.RenderEditor();
}

void ModelComponent::RenderEditorComponentTree(const uint8_t layer)
{
    ImGui::Indent( layer * 16 );
    ImGui::Text(name);
    ImGui::Unindent( layer * 16 );
    for (uint32_t child : children)
    {
        // TODO Get Component from library by UUID
        Component* childComponent = ComponentUtils::CreateEmptyComponent(COMPONENT_MODEL, LCG().IntFast(), uuid);

        if (ImGui::CollapsingHeader("ComponentName"))   // TODO Add name from component
        {
            childComponent->RenderEditorComponentTree(layer + 1);
        }
    }
}

void ModelComponent::Update()
{
}

void ModelComponent::LoadModel(uint32_t modelUUID)
{
    currentModelUUID = modelUUID;
}