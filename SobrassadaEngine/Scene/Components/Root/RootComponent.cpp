#include "RootComponent.h"

#include "Application.h"
#include "EditorUIModule.h"
#include "SceneModule.h"
#include "imgui.h"
#include "Scene/Components/Standalone/ModelComponent.h"

#include <Algorithm/Random/LCG.h>

RootComponent::RootComponent(const uint32_t uuid, const uint32_t uuidParent, const char* name)
        : Component(uuid, uuidParent, uuid, name)
{
    selectedUUID = uuid;    // TODO Other components don´t have the correct selectedUUID
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

void RootComponent::RenderComponentEditor()
{
    Component* selectedComponent = App->GetSceneModule()->gameComponents[selectedUUID];
    
    ImGui::Begin("Inspector");    // TODO Add bool parameter at the end to then unselect the component (Add isSelected property to component?)

    //ImGui::InputText(name, test, 10, ImGuiInputTextFlags_None);
    ImGui::Text(name);
    ImGui::SameLine();
    ImGui::Checkbox("Enabled", &enabled); // TODO Call Enable() / Disable() instead of setting the value directly
    
    if (ImGui::Button("Add Component")) // TODO Get selected component to add the new one at the correct location (By UUID)
    {
        CreateComponent(COMPONENT_MODEL); // TODO Add selection table dropdown to select which component to add
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove Component")) 
    {
        if (selectedComponent != nullptr)
        {
            Component* selectedParentComponent = App->GetSceneModule()->gameComponents[selectedComponent->GetUUIDParent()];
            if (selectedParentComponent != nullptr)  selectedParentComponent->RemoveComponent(selectedUUID);
            // TODO Make sure component gets fully deleted if not used anywhere else, delete mesh from memory for example
            // TODO Remove children components as well or assign them to the parent. To decide!
        }
    }
    
    RenderEditorComponentTree(selectedUUID);
    
    ImGui::Spacing();

    ImGui::SeparatorText("Modules Configuration");
    
    if (selectedComponent != nullptr)  selectedComponent->RenderEditorInspector();

    ImGui::End();
}

void RootComponent::RenderEditorComponentTree(const uint32_t selectedComponentUUID)
{
    ImGui::SeparatorText("Component hierarchy");
    
    ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (selectedUUID == uuid)
    {
        base_flags |= ImGuiTreeNodeFlags_Selected;
    }
    const bool isExpanded = ImGui::TreeNodeExV((void*) uuid, base_flags, name, nullptr);
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        SetSelectedComponent(uuid);
    
    if (isExpanded) 
    {
        for (uint32_t child : children)
        {
            Component* childComponent = App->GetSceneModule()->gameComponents[child];

            if (childComponent != nullptr)  childComponent->RenderEditorComponentTree(selectedUUID);
        }
        ImGui::TreePop();
    }
}

void RootComponent::RenderEditorInspector()
{
    App->GetEditorUIModule()->RenderTransformModifier(localTransform, globalTransform);
}

void RootComponent::Update()
{
}

void RootComponent::SetSelectedComponent(const uint32_t componentUUID)
{
    selectedUUID = componentUUID;
}

bool RootComponent::CreateComponent(const ComponentType componentType)
{
    // TODO Call library to create the component with an id instead
    Component* createdComponent = ComponentUtils::CreateEmptyComponent(componentType, LCG().IntFast(), selectedUUID, uuid);
    Component* selectedComponent = App->GetSceneModule()->gameComponents[selectedUUID];
    if (createdComponent != nullptr && selectedComponent != nullptr) {
        App->GetSceneModule()->gameComponents[createdComponent->GetUUID()] = createdComponent;
        
        selectedComponent->AddComponent(createdComponent->GetUUID());
        return true;
    }
    return false;
}