#include "Component.h"

#include "Application.h"
#include "ComponentUtils.h"
#include "EditorUIModule.h"
#include "SceneModule.h"
#include "imgui.h"
#include "Root/RootComponent.h"

Component::Component(const uint32_t uuid, const uint32_t uuidParent, const uint32_t uuidRoot, const char* name, const Transform& parentGlobalTransform):
uuid(uuid), uuidParent(uuidParent), uuidRoot(uuidRoot), name(name), enabled(true), globalTransform(parentGlobalTransform){}

Component::~Component(){
    for (const uint32_t child : children)
    {
        delete App->GetSceneModule()->gameComponents[child];
        App->GetSceneModule()->gameComponents.erase(child);
    }
}

void Component::Enable()
{
    enabled = true;
}

void Component::Render()
{
    for (uint32_t childUUID: children)
    {
        Component* child = App->GetSceneModule()->gameComponents[childUUID];
        if (child != nullptr) child->Render();
    }
}

void Component::Disable()
{
    enabled = false;
}

bool Component::AddChildComponent(const uint32_t componentUUID)
{
    children.push_back(componentUUID);
    return true;
}

bool Component::RemoveChildComponent(const uint32_t componentUUID){
    if (const auto it = std::find(children.begin(), children.end(), componentUUID); it != children.end())
    {
        children.erase(it);
        return true;
    }
    return false;
}

bool Component::DeleteChildComponent(const uint32_t componentUUID)
{
    if (const auto it = std::find(children.begin(), children.end(), componentUUID); it != children.end())
    {
        children.erase(it);
        delete App->GetSceneModule()->gameComponents[componentUUID];
        App->GetSceneModule()->gameComponents.erase(componentUUID);
        
        return true;
    }
    
    return false;
}

void Component::RenderEditorInspector()
{
    ImGui::Text(name);
    ImGui::Separator();
    if (App->GetEditorUIModule()->RenderTransformModifier(localTransform, globalTransform, uuidParent))
    {
        TransformUpdated();
    }
}

void Component::RenderEditorComponentTree(const uint32_t selectedComponentUUID)
{
    ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (selectedComponentUUID == uuid)
    {
        base_flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (children.empty())
    {
        base_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    ImGui::PushID(uuid);
    const bool isExpanded = ImGui::TreeNodeEx(name, base_flags);
    ImGui::PopID();
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        RootComponent* rootComponent = App->GetSceneModule()->GetRootComponent();
        if (rootComponent != nullptr)
        {
            rootComponent->SetSelectedComponent(uuid);
        }
    }

    HandleDragNDrop();
    
    if (isExpanded && !children.empty()) 
    {
        for (uint32_t child : children)
        {
            Component* childComponent = App->GetSceneModule()->gameComponents[child];

            if (childComponent != nullptr)  childComponent->RenderEditorComponentTree(selectedComponentUUID);
        }
        ImGui::TreePop();
    }
}

void Component::ParentGlobalTransformUpdated(const Transform &parentGlobalTransform)
{
    globalTransform = parentGlobalTransform + localTransform;
    TransformUpdated();
}

void Component::HandleDragNDrop(){
    if (ImGui::BeginDragDropSource())
    {
        GLOG("Starting component drag n drop for uuid : %d", uuid)
        ImGui::SetDragDropPayload("ComponentTreeNode", &uuid, sizeof(uint32_t));
        ImGui::Text(name);
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ComponentTreeNode"))
        {
            const uint32_t draggedUUID = *(const uint32_t*)payload->Data;
            GLOG("Receiving component drag n drop for uuid : %d", draggedUUID)
            if (draggedUUID != uuid && draggedUUID != uuidParent)
            {
                Component* draggedComponent = App->GetSceneModule()->gameComponents[draggedUUID];
                if (draggedComponent != nullptr)
                {
                    Component* parentDraggedComponent = App->GetSceneModule()->gameComponents[draggedComponent->GetUUIDParent()];
                    if (parentDraggedComponent != nullptr)
                    {
                        parentDraggedComponent->RemoveChildComponent(draggedUUID);
                        draggedComponent->SetUUIDParent(uuid); // TODO Add set parent uuid into the AddChildComponent function
                        AddChildComponent(draggedUUID);
                        draggedComponent->localTransform.Set(draggedComponent->globalTransform - globalTransform);
                        draggedComponent->TransformUpdated();
                    }
                }
            }
            
            ImGui::EndDragDropTarget();
        }
        
    }
}

void Component::TransformUpdated(){
    for (uint32_t child : children)
    {
        Component* childComponent = App->GetSceneModule()->gameComponents[child];

        if (childComponent != nullptr)  childComponent->ParentGlobalTransformUpdated(globalTransform);
    }
}
