﻿#include "RootComponent.h"

#include "Application.h"
#include "EditorUIModule.h"
#include "SceneModule.h"
#include "imgui.h"
#include "Scene/Components/Standalone/MeshComponent.h"
#include "Scene/GameObjects/GameObject.h"

#include <Algorithm/Random/LCG.h>

RootComponent::RootComponent(const UID uid, const UID uidParent, const Transform& parentGlobalTransform)
        : Component(uid, uidParent, uid, "Root component", COMPONENT_ROOT, parentGlobalTransform)
{
    selectedUID = uid;  
}

RootComponent::RootComponent(const rapidjson::Value &initialState) : Component(initialState)
{
    selectedUID = uid;
    mobilitySettings = initialState["Mobility"].GetInt();
}


RootComponent::~RootComponent(){
    Component::~Component();
}

void RootComponent::Save(rapidjson::Value &targetState, rapidjson::Document::AllocatorType &allocator) const
{
    Component::Save(targetState, allocator);
    targetState.AddMember("Type", COMPONENT_ROOT, allocator);
    targetState.AddMember("Mobility", mobilitySettings, allocator);
}

AABB & RootComponent::TransformUpdated(const Transform &parentGlobalTransform)
{
    AABB& result = Component::TransformUpdated(parentGlobalTransform);

    AABBUpdatable* parentObject = GetParent();
    if (parentObject != nullptr)
    {
        parentObject->ComponentGlobalTransformUpdated();
    }

    return result;
}

void RootComponent::RenderComponentEditor()
{
    Component* selectedComponent = App->GetSceneModule()->GetComponentByUID(selectedUID);
    
    if (!ImGui::Begin("Inspector", &App->GetEditorUIModule()->inspectorMenu))
    {
        ImGui::End();
        return;
    }

    //ImGui::InputText(name, test, 10, ImGuiInputTextFlags_None);
    ImGui::Text(name);
    
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

    if (selectedUID != uid)
    {
        ImGui::SameLine();
        if (ImGui::Button("Remove Component")) 
        {
            if (selectedComponent != nullptr)
            {
                Component* selectedParentComponent = App->GetSceneModule()->GetComponentByUID(selectedComponent->GetUIDParent());
                if (selectedParentComponent != nullptr)
                {
                    selectedParentComponent->DeleteChildComponent(selectedUID);
                    selectedUID = uid;
                    selectedComponent = nullptr;
                }
            }
        }
    }
    
    RenderEditorComponentTree(selectedUID);
    
    ImGui::Spacing();

    ImGui::SeparatorText("Component configuration");

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
    ImGui::BeginChild("ComponentInspectorWrapper", ImVec2(0, 200), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY);
    
    if (selectedComponent != nullptr)  selectedComponent->RenderEditorInspector();
    
    ImGui::EndChild();
    ImGui::PopStyleVar();
    
    ImGui::End();
}

void RootComponent::RenderEditorComponentTree(const UID selectedComponentUID)
{
    ImGui::SeparatorText("Component hierarchy");
   
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
    ImGui::BeginChild("ComponentHierarchyWrapper", ImVec2(0, 200), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY);
    ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (selectedComponentUID == uid)
    {
        base_flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (children.empty())
    {
        base_flags |= ImGuiTreeNodeFlags_Leaf;
    }

    const bool isExpanded = ImGui::TreeNodeEx((void*) uid, base_flags, name);
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        SetSelectedComponent(uid);

    HandleDragNDrop();
    
    if (isExpanded) 
    {
        for (Component* childComponent : GetChildComponents())
        {
            childComponent->RenderEditorComponentTree(selectedUID);
        }
        ImGui::TreePop();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
}

void RootComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();
    if (enabled)
    {
        // Casting to use ImGui to set values and at the same type keep the enum type for the variable
        ImGui::SeparatorText("Mobility");
        ImGui::RadioButton("Static", &mobilitySettings, STATIC);
        ImGui::SameLine();
        ImGui::RadioButton("Dynamic", &mobilitySettings, DYNAMIC);
    }
}

void RootComponent::RenderGuizmo()
{
    if (selectedUID == uid)
    {
        if (App->GetEditorUIModule()->RenderImGuizmo(localTransform))
        {
            AABBUpdatable* parentGameObject = GetParent();
            if (parentGameObject != nullptr)
            {
                OnTransformUpdate(parentGameObject->GetParentGlobalTransform()); // Step up two parents to get the correct transform
            }
        }
    }
}

void RootComponent::Update()
{
}

void RootComponent::SetSelectedComponent(const UID componentUID)
{
    selectedUID = componentUID;
}

bool RootComponent::CreateComponent(const ComponentType componentType)
{
    // TODO Call library to create the component with an id instead
    Component* selectedComponent = App->GetSceneModule()->GetComponentByUID(selectedUID);
    if (selectedComponent != nullptr) {
        Component* createdComponent = ComponentUtils::CreateEmptyComponent(componentType, LCG().IntFast(), selectedUID, uid, selectedComponent->GetGlobalTransform());
        if (createdComponent != nullptr)
        {
            App->GetSceneModule()->AddComponent(createdComponent->GetUID(), createdComponent);
        
            selectedComponent->AddChildComponent(createdComponent->GetUID());
            return true;
        }
    }
    return false;
}
