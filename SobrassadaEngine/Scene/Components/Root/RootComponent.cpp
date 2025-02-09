#include "RootComponent.h"

#include "Application.h"
#include "EditorUIModule.h"
#include "SceneModule.h"
#include "imgui.h"
#include "Scene/Components/Standalone/MeshComponent.h"

#include <Algorithm/Random/LCG.h>

RootComponent::RootComponent(const uint32_t uuid, const uint32_t uuidParent, const char* name, const Transform& parentGlobalTransform)
        : Component(uuid, uuidParent, uuid, name, parentGlobalTransform)
{
    selectedUUID = uuid;    // TODO Other components don´t have the correct selectedUUID
}

RootComponent::~RootComponent(){
}

bool RootComponent::AddChildComponent(const uint32_t componentUUID)
{
    // TODO Load component from storage
    // TODO Make sure passed componentUUID encodes a standalone component
    return Component::AddChildComponent(componentUUID);
}

bool RootComponent::RemoveChildComponent(const uint32_t componentUUID)
{
    return Component::RemoveChildComponent(componentUUID);
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
        ImGui::OpenPopup("ComponentSelection");
    }

    if (ImGui::BeginPopup("ComponentSelection"))
    {
        static char searchText[255] = "";
        ImGui::InputText("Search", searchText, 255);
        
        ImGui::Separator();
        if (ImGui::BeginListBox("##ComponentList", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing())))
        {
            for ( const auto &componentPair : standaloneComponents ) {
                {
                    if (componentPair.first.find(searchText) != std::string::npos)
                    {
                        if (ImGui::Selectable(componentPair.first.c_str(), false))
                        {
                            CreateComponent(componentPair.second);
                            ImGui::CloseCurrentPopup();
                        }
                            
                    }
                }
            }
            ImGui::EndListBox();
        }
        ImGui::EndPopup();
    }

    ImGui::ShowDemoWindow();

    if (selectedUUID != uuid)
    {
        ImGui::SameLine();
        if (ImGui::Button("Remove Component")) 
        {
            if (selectedComponent != nullptr)
            {
                Component* selectedParentComponent = App->GetSceneModule()->gameComponents[selectedComponent->GetUUIDParent()];
                if (selectedParentComponent != nullptr)
                {
                    selectedParentComponent->RemoveChildComponent(selectedUUID);
                    selectedUUID = uuid;
                    selectedComponent = nullptr;
                }
            }
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
    if (selectedComponentUUID == uuid)
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
    Component::RenderEditorInspector();
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
    Component* selectedComponent = App->GetSceneModule()->gameComponents[selectedUUID];
    if (selectedComponent != nullptr) {
        Component* createdComponent = ComponentUtils::CreateEmptyComponent(componentType, LCG().IntFast(), selectedUUID, uuid, selectedComponent->GetGlobalTransform());
        if (createdComponent != nullptr)
        {
            App->GetSceneModule()->gameComponents[createdComponent->GetUUID()] = createdComponent;
        
            selectedComponent->AddChildComponent(createdComponent->GetUUID());
            return true;
        }
    }
    return false;
}
