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
#include <string>
#include <Math/float4x4.h>

Component::Component(const UID uid, const UID uidParent, const UID uidRoot, const char* initName, int type, const Transform& parentGlobalTransform):
uid(uid), uidParent(uidParent), uidRoot(uidRoot), type(type), enabled(true), globalTransform(parentGlobalTransform)
{
    localComponentAABB.SetNegativeInfinity();
    globalComponentAABB.SetNegativeInfinity();
    memcpy(name, initName, strlen(initName));
}

Component::Component(const rapidjson::Value &initialState): uid(initialState["UID"].GetUint64()), uidRoot(initialState["RootUID"].GetUint64()),
type(initialState["Type"].GetInt())
{
    uidParent = initialState["ParentUID"].GetUint64();
    enabled = initialState["Enabled"].GetBool();
    
    if (initialState.HasMember("LocalTransform") && initialState["LocalTransform"].IsArray() && initialState["LocalTransform"].Size() == 9)
    {
        const rapidjson::Value &initLocalTransform = initialState["LocalTransform"];

        localTransform = Transform(float3(initLocalTransform[0].GetFloat(), initLocalTransform[1].GetFloat(), initLocalTransform[2].GetFloat()),
            float3(initLocalTransform[3].GetFloat(), initLocalTransform[4].GetFloat(), initLocalTransform[5].GetFloat()),
            float3(initLocalTransform[6].GetFloat(), initLocalTransform[7].GetFloat(), initLocalTransform[8].GetFloat()));
    }

    if (initialState.HasMember("Children") && initialState["Children"].IsArray())
    {
        const rapidjson::Value &initChildren = initialState["Children"];
        
        for (rapidjson::SizeType i = 0; i < initChildren.Size(); i++)
        {
            children.push_back(initChildren[i].GetUint64());
        }
    }
    const char* initName = initialState["Name"].GetString();
    memcpy(name, initName, strlen(initName));

    // TODO Call OnTransformUpdated on the root gameobject once every component and gameobject is initiated. That will
    // TODO generate the missing global transforms and aabbs for every gameobject and component

    // TODO Include for document is messed up, include <document.h> doesnt work, only #include <Libs/rapidjson/document.h>
}

Component::~Component(){
    for (const Component* child : GetChildComponents())
    {
        App->GetSceneModule()->gameComponents.erase(child->GetUID());
        delete child;
    }
}

void Component::Save(rapidjson::Value &targetState, rapidjson::Document::AllocatorType &allocator) const
{
    targetState.AddMember("UID", uid, allocator);
    targetState.AddMember("ParentUID", uidParent, allocator);
    targetState.AddMember("RootUID", uidRoot, allocator);
    targetState.AddMember("Type", type, allocator);

    rapidjson::Value valLocalTransform(rapidjson::kArrayType);
    valLocalTransform.PushBack(localTransform.position.x, allocator).PushBack(localTransform.position.y, allocator).
    PushBack(localTransform.position.z, allocator).PushBack(localTransform.rotation.x, allocator).
    PushBack(localTransform.rotation.y, allocator).PushBack(localTransform.rotation.z, allocator).
    PushBack(localTransform.scale.x, allocator).PushBack(localTransform.scale.y, allocator).
    PushBack(localTransform.scale.z, allocator);
    
    targetState.AddMember("LocalTransform", valLocalTransform, allocator);

    rapidjson::Value valChildren(rapidjson::kArrayType);

    for (const UID child : children)
    {
        valChildren.PushBack(child, allocator);
    }

    targetState.AddMember("Children", valChildren, allocator);
    targetState.AddMember("Enabled", enabled, allocator);
    targetState.AddMember("Name", rapidjson::Value(std::string(name).c_str(), allocator), allocator);
}


void Component::Render()
{
    if (enabled)
    {
        for (Component* child: GetChildComponents())
        {
            child->Render();
        }
    }
}

bool Component::AddChildComponent(const UID componentUID)
{
    children.push_back(componentUID);
    return true;
}

bool Component::RemoveChildComponent(const UID componentUID){
    if (const auto it = std::find(children.begin(), children.end(), componentUID); it != children.end())
    {
        children.erase(it);
        return true;
    }
    return false;
}

bool Component::DeleteChildComponent(const UID componentUID)
{
    if (const auto it = std::find(children.begin(), children.end(), componentUID); it != children.end())
    {
        children.erase(it);
        delete App->GetSceneModule()->gameComponents[componentUID]; // More efficient than finding the child pointer in childComponents
        App->GetSceneModule()->gameComponents.erase(componentUID);
        
        return true;
    }
    
    return false;
}

void Component::RenderEditorInspector()
{
    ImGui::InputText("Name", name, sizeof(name));
    ImGui::SameLine();
    ImGui::Checkbox("Enabled", &enabled);
    if (enabled)
    {
        ImGui::Separator();

        const Transform& parentTransform = GetParentGlobalTransform();
        if (App->GetEditorUIModule()->RenderTransformWidget(localTransform, globalTransform, parentTransform))
        {
            OnTransformUpdate(parentTransform);
        }
    }
}

void Component::RenderEditorComponentTree(const UID selectedComponentUID)
{
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
    {
        RootComponent* rootComponent = GetRootComponent();
        if (rootComponent != nullptr)
        {
            rootComponent->SetSelectedComponent(uid);
        }
                
    }

    HandleDragNDrop();
    
    if (isExpanded) 
    {
        for (Component* childComponent : GetChildComponents())
        {
           childComponent->RenderEditorComponentTree(selectedComponentUID);
        }
        ImGui::TreePop();
    }
}

const Transform& Component::GetParentGlobalTransform()
{
    AABBUpdatable* parentObject = GetParent();
    if (parentObject != nullptr)
    {
        return parentObject->GetGlobalTransform();
    }
    return Transform::identity;
}

void Component::HandleDragNDrop(){
    if (ImGui::BeginDragDropSource())
    {
        GLOG("Starting component drag n drop for uuid : %d", uid)
        ImGui::SetDragDropPayload("ComponentTreeNode", &uid, sizeof(UID));
        ImGui::Text(name);
        ImGui::EndDragDropSource();
    }
    

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ComponentTreeNode"))
        {
            const UID draggedUID = *(const UID*)payload->Data;
            GLOG("Receiving component drag n drop for uuid : %d", draggedUID)
            if (draggedUID != uid)
            {
                Component* draggedComponent = App->GetSceneModule()->gameComponents[draggedUID];
                if (draggedComponent != nullptr)
                {
                    Component* parentDraggedComponent = App->GetSceneModule()->gameComponents[draggedComponent->GetUIDParent()];
                    if (parentDraggedComponent != nullptr)
                    {
                        parentDraggedComponent->RemoveChildComponent(draggedUID);
                        parentDraggedComponent->PassAABBUpdateToParent();
                        draggedComponent->SetUIDParent(uid); 
                        AddChildComponent(draggedUID);
                        draggedComponent->localTransform.Set(draggedComponent->globalTransform - globalTransform);
                        draggedComponent->OnTransformUpdate(globalTransform);
                    }
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void Component::SetUIDParent(UID newUIDParent)
{
    uidParent = newUIDParent;
    parent = nullptr;
}

void Component::OnTransformUpdate(const Transform &parentGlobalTransform)
{
    TransformUpdated(parentGlobalTransform);

    AABBUpdatable* parentObject = GetParent();
    if (parentObject != nullptr)
    {
        parentObject->PassAABBUpdateToParent();
    }
}

AABB& Component::TransformUpdated(const Transform &parentGlobalTransform)
{
    globalTransform = parentGlobalTransform + localTransform;

    CalculateLocalAABB();

    for (Component* childComponent : GetChildComponents())
    {
        globalComponentAABB.Enclose(childComponent->TransformUpdated(globalTransform));
    }

    return globalComponentAABB;
}

RootComponent * Component::GetRootComponent()
{
    if (rootComponent == nullptr)
    {
        rootComponent = dynamic_cast<RootComponent* >(App->GetSceneModule()->gameComponents[uidRoot]);
        if (rootComponent == nullptr)
        {
            //GLOG("Could not load parent with UID: %s - Object does not exist", uidRoot)
        }
    }
    return rootComponent;
}

AABBUpdatable * Component::GetParent()
{
    if (parent == nullptr)
    {
        parent = App->GetSceneModule()->GetTargetForAABBUpdate(uidParent);
        if (parent == nullptr)
        {
           // GLOG("Could not load parent with UID: %s - Object does not exist", uidParent)
        }
    }
    return parent;
}

std::vector<Component *> & Component::GetChildComponents()
{
    if (children.size() != childComponents.size())
    {
        childComponents.clear();
        for (UID child : children)
        {
            Component* childComponent = App->GetSceneModule()->gameComponents[child];
            if (childComponent != nullptr)
            {
                childComponents.push_back(childComponent);
            } else
            {
               // GLOG("Could not load component with UID: %s - Component does not exist", child)
            }
        }
    }
    return childComponents;
}

void Component::PassAABBUpdateToParent()
{
    CalculateLocalAABB();
    
    for (Component* childComponent : GetChildComponents())
    {
        globalComponentAABB.Enclose(childComponent->GetGlobalAABB());
    }

    GLOG("AABB updated")
    GLOG("AABB: (%f, %f, %f), (%f, %f, %f)", globalComponentAABB.minPoint.x, globalComponentAABB.minPoint.y, globalComponentAABB.minPoint.z,
       globalComponentAABB.maxPoint.x, globalComponentAABB.maxPoint.y, globalComponentAABB.maxPoint.z)

    AABBUpdatable* parentObject = GetParent();
    if (parentObject != nullptr)
    {
        parentObject->PassAABBUpdateToParent();
    }
}

void Component::CalculateLocalAABB()
{
    OBB globalComponentOBB = OBB(localComponentAABB);
    globalComponentOBB.Transform(float4x4::FromTRS(
                    globalTransform.position,
                    Quat::FromEulerXYZ(globalTransform.rotation.x, globalTransform.rotation.y, globalTransform.rotation.z),
                    globalTransform.scale)); // TODO Testing once the aabb debug renderer is available
    
    
    globalComponentAABB = AABB(globalComponentOBB);
}
