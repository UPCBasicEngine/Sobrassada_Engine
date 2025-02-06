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
    App->GetEditorUIModule()->RenderTransformModifier(localTransform, globalTransform);
}

void ModelComponent::RenderEditorComponentTree(const uint32_t selectedComponentUUID)
{
    ImGui::Indent( 16 );

    ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (selectedComponentUUID == uuid)
    {
        base_flags |= ImGuiTreeNodeFlags_Selected;
    }
    const bool isExpanded = ImGui::TreeNodeExV((void*) uuid, base_flags, name, nullptr);
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        RootComponent* rootComponent = dynamic_cast<RootComponent *>(App->GetSceneModule()->gameComponents[uuidRoot]);
        if (rootComponent != nullptr)
        {
            rootComponent->SetSelectedComponent(uuid);
        }
    }
        
    //ImGui::PopID();
    //ImGui::SameLine();
    //selected = ImGui::Selectable(": Model", &selected); 
    if (isExpanded) 
    {
        for (uint32_t child : children)
        {
            Component* childComponent = App->GetSceneModule()->gameComponents[child];

            if (childComponent != nullptr)  childComponent->RenderEditorComponentTree(selectedComponentUUID);
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