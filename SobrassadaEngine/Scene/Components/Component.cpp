#include "Component.h"

#include "Application.h"
#include "ComponentUtils.h"
#include "EditorUIModule.h"
#include "SceneModule.h"
#include "imgui.h"
#include "Root/RootComponent.h"
#include "GameObject.h"
#include "debug_draw.hpp"

#include <Geometry/OBB.h>
#include <Math/Quat.h>

Component::Component(const uint32_t uuid, const uint32_t uuidParent, const uint32_t uuidRoot, const char* name, const Transform& parentGlobalTransform):
uuid(uuid), uuidParent(uuidParent), uuidRoot(uuidRoot), name(name), enabled(true), globalTransform(parentGlobalTransform)
{
    localComponentAABB.SetNegativeInfinity();
    globalComponentAABB.SetNegativeInfinity();
}

Component::~Component(){
    for (const uint32_t child : children)
    {
        delete App->GetSceneModule()->gameComponents[child];
        App->GetSceneModule()->gameComponents.erase(child);
    }
}

void Component::Render()
{
    if (enabled)
    {
        for (uint32_t childUUID: children)
        {
            Component* child = App->GetSceneModule()->gameComponents[childUUID];
            if (child != nullptr) child->Render();
        }
    }
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
    ImGui::SameLine();
    ImGui::Checkbox("Enabled", &enabled);
    if (enabled)
    {
        ImGui::Separator();
        if (App->GetEditorUIModule()->RenderTransformModifier(localTransform, globalTransform, uuidParent))
        {
            AABBUpdatable* parent = App->GetSceneModule()->GetTargetForAABBUpdate(uuidParent);
            if (parent != nullptr)
            {
                OnTransformUpdate(parent->GetGlobalTransform());
            } else
            {
                OnTransformUpdate(Transform());
            }
        }
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
        base_flags |= ImGuiTreeNodeFlags_Leaf;
    }
    const bool isExpanded = ImGui::TreeNodeEx((void*) uuid, base_flags, name);
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        GameObject* selectedGameObject = App->GetSceneModule()->GetSeletedGameObject();
        if (selectedGameObject != nullptr)
        {
            RootComponent* rootComponent = selectedGameObject-> GetRootComponent();
            if (rootComponent != nullptr)
            {
                rootComponent->SetSelectedComponent(uuid);
            }
        }
        
    }

    HandleDragNDrop();
    
    if (isExpanded) 
    {
        for (uint32_t child : children)
        {
            Component* childComponent = App->GetSceneModule()->gameComponents[child];

            if (childComponent != nullptr)  childComponent->RenderEditorComponentTree(selectedComponentUUID);
        }
        ImGui::TreePop();
    }
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
            if (draggedUUID != uuid)
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
                        draggedComponent->OnTransformUpdate(globalTransform);
                    }
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void Component::OnTransformUpdate(const Transform &parentGlobalTransform)
{
    TransformUpdated(globalTransform);
    
    AABBUpdatable* parent = App->GetSceneModule()->GetTargetForAABBUpdate(uuidParent);
    if (parent != nullptr)
    {
        parent->PassAABBUpdateToParent();
    }
}

AABB& Component::TransformUpdated(const Transform &parentGlobalTransform)
{
    globalTransform = parentGlobalTransform + localTransform;

    CalculateGlobalAABB();

    return globalComponentAABB;
}

void Component::PassAABBUpdateToParent()
{
    CalculateGlobalAABB();

    GLOG("AABB updated")
   GLOG("AABB: (%f, %f, %f), (%f, %f, %f)", globalComponentAABB.minPoint.x, globalComponentAABB.minPoint.y, globalComponentAABB.minPoint.z,
       globalComponentAABB.maxPoint.x, globalComponentAABB.maxPoint.y, globalComponentAABB.maxPoint.z)

    AABBUpdatable* parent = App->GetSceneModule()->GetTargetForAABBUpdate(uuidParent);
    if (parent != nullptr)
    {
        parent->PassAABBUpdateToParent();
    }
}

void Component::CalculateGlobalAABB()
{
    OBB globalComponentOBB = OBB(localComponentAABB);
    globalComponentOBB.Transform(float4x4::FromTRS(
                    globalTransform.position,
                    Quat::FromEulerXYZ(globalTransform.rotation.x, globalTransform.rotation.y, globalTransform.rotation.z),
                    globalTransform.scale)); // TODO Testing once the aabb debug renderer is available
    
    
    globalComponentAABB = AABB(globalComponentOBB);
    
    for (uint32_t child : children)
    {
        Component* childComponent = App->GetSceneModule()->gameComponents[child];

        if (childComponent != nullptr)
        {
            globalComponentAABB.Enclose(childComponent->GetGlobalAABB());
        }
    }
}
