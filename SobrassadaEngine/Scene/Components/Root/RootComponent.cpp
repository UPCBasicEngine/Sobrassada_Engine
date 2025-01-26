#include "RootComponent.h"

#include "imgui.h"
#include "Scene/Components/Standalone/ModelComponent.h"

#include <Algorithm/Random/LCG.h>

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
    ImGui::Begin("Inspector");    // TODO Add bool parameter at the end to then unselect the component (Add isSelected property to component?)

    //ImGui::InputText(name, test, 10, ImGuiInputTextFlags_None);
    ImGui::Text(name);
    ImGui::SameLine();
    ImGui::Checkbox("Enabled", &enabled); // TODO Call Enable() / Disable() instead of setting the value directly
    ImGui::SameLine();
    if (ImGui::Button("Add Component")) // TODO Get selected component to add the new one at the correct location (By UUID)
    {
        CreateComponent(COMPONENT_MODEL); // TODO Add selection table dropdown to select which component to add
    }
    
    ImGui::SeparatorText("Component hierarchy");

    ImGui::Text(name);
    
    for (uint32_t child : children)
    {
        // TODO Get Component from library by UUID
        Component* childComponent = ComponentUtils::CreateEmptyComponent(COMPONENT_MODEL, LCG().IntFast(), uuid);

        if (ImGui::CollapsingHeader("ComponentName"))   // TODO Add name from component
        {
            childComponent->RenderEditorComponentTree(1);
        }
    }
    
    ImGui::Spacing();

    ImGui::SeparatorText("Modules Configuration");

    ImGui::Text("Local transform");
    localTransform.RenderEditor();
    
    ImGui::Spacing();
    
    ImGui::Text("Global transform");
    globalTransform.RenderEditor();

    for (uint32_t child : children)
    {
        // TODO Get Component from library by UUID
        Component* childComponent = ComponentUtils::CreateEmptyComponent(COMPONENT_MODEL, LCG().IntFast(), uuid);

        if (ImGui::CollapsingHeader("ComponentName"))   // TODO Add name from component
        {
            childComponent->RenderEditorInspector();
        }
    }
    

    ImGui::End();
}

void RootComponent::Update()
{
}