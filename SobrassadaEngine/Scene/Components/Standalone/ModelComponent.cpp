#include "ModelComponent.h"

#include "Application.h"
#include "EditorUIModule.h"
#include "imgui.h"

#include <Algorithm/Random/LCG.h>

ModelComponent::ModelComponent(const uint32_t uuid, const uint32_t ownerUUID, const char* name)
        : Component(uuid, ownerUUID, name)
{
}

void ModelComponent::RenderEditorInspector()
{
    App->GetEditorUIModule()->RenderTransformModifier(localTransform, globalTransform);
}

void ModelComponent::RenderEditorComponentTree(const uint8_t layer)
{
    ImGui::Indent( 16 );
    
    const bool isExpanded = ImGui::TreeNode(name);
    ImGui::SameLine();
    ImGui::Selectable(": Model", false); 
    if (isExpanded) 
    {
        for (uint32_t child : children)
        {
            // TODO Get Component from library by UUID
            Component* childComponent = ComponentUtils::CreateEmptyComponent(COMPONENT_MODEL, LCG().IntFast(), uuid);

            childComponent->RenderEditorComponentTree(1);
        }
    }
    
    ImGui::Unindent( 16 );
}

void ModelComponent::Update()
{
}

void ModelComponent::LoadModel(uint32_t modelUUID)
{
    currentModelUUID = modelUUID;
}