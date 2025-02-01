#include "ModelComponent.h"

#include "Application.h"
#include "EditorUIModule.h"
#include "SceneModule.h"
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

void ModelComponent::RenderEditorComponentTree()
{
    ImGui::Indent( 16 );

    ImGui::PushID(uuid);
    const bool isExpanded = ImGui::TreeNodeExV((void*) nullptr, ImGuiTreeNodeFlags_Framed, name, nullptr);
    ImGui::PopID();
    ImGui::SameLine();
    ImGui::Selectable(": Model", false); 
    if (isExpanded) 
    {
        for (uint32_t child : children)
        {
            Component* childComponent = App->GetSceneModule()->gameComponents[child];

            if (childComponent != nullptr)  childComponent->RenderEditorComponentTree();
        }
        ImGui::TreePop();
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