#include "GameObject.h"

#include "Application.h"
#include "EditorUIModule.h"
#include "Root/RootComponent.h"
#include "SceneModule.h"

#include "imgui.h"

#include <Algorithm/Random/LCG.h>

GameObject::GameObject(std::string name) : name(name)
{
    uuid       = LCG().IntFast();
    parentUUID = INVALID_UUID;
    globalAABB.SetNegativeInfinity();
    CreateRootComponent();
}

GameObject::GameObject(UID parentUUID, std::string name) : parentUUID(parentUUID), name(name)
{
    uuid = LCG().IntFast();
    globalAABB.SetNegativeInfinity();
    CreateRootComponent();
}

GameObject::GameObject(UID parentUUID, std::string name, UID rootComponentUID) : parentUUID(parentUUID), name(name)
{
    rootComponent = dynamic_cast<RootComponent *>(App->GetSceneModule()->gameComponents[rootComponentUID]);
}

GameObject::~GameObject()
{
    App->GetSceneModule()->gameComponents.erase(rootComponent->GetUID());
    delete rootComponent;
    rootComponent = nullptr;
}

bool GameObject::CreateRootComponent()
{

    rootComponent = dynamic_cast<RootComponent *>(
        ComponentUtils::CreateEmptyComponent(COMPONENT_ROOT, LCG().IntFast(), uuid, -1, Transform())
    ); // TODO Add the gameObject UUID as parent?

    // TODO Replace parentUUID above with the UUID of this gameObject
    App->GetSceneModule()->gameComponents[rootComponent->GetUID()] = rootComponent;
    return true;
}

bool GameObject::AddGameObject(UID gameObjectUUID)
{
    if (std::find(children.begin(), children.end(), gameObjectUUID) == children.end())
    {
        children.push_back(gameObjectUUID);
        return true;
    }
    return false;
}

bool GameObject::RemoveGameObject(UID gameObjectUUID)
{
    if (const auto it = std::find(children.begin(), children.end(), gameObjectUUID); it != children.end())
    {
        children.erase(it);
        return true;
    }
    return false;
}

void GameObject::OnEditor() {}

void GameObject::SaveToLibrary() {}

void GameObject::RenderHierarchyNode(UID &selectedGameObjectUUID) 
{
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    
    bool hasChildren         = !children.empty();
    
    if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    if (selectedGameObjectUUID == uuid)
    {

        flags |= ImGuiTreeNodeFlags_Selected;
    }

    ImGui::PushID(uuid);
    bool nodeOpen = ImGui::TreeNodeEx(name.c_str(), flags);

    HandleNodeClick(selectedGameObjectUUID);
    RenderContextMenu();

    if (nodeOpen && hasChildren)
    {
        for (UID childUUID : children)
        {
            GameObject *childGameObject = App->GetSceneModule()->GetGameObjectByUUID(childUUID);
            if (childUUID != uuid)
            {
                childGameObject->RenderHierarchyNode(selectedGameObjectUUID);
            }
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}

void GameObject::HandleNodeClick(UID &selectedGameObjectUUID) 
{
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        selectedGameObjectUUID = uuid;
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        selectedGameObjectUUID = uuid;
        ImGui::OpenPopup(("Game Object Context Menu##" + std::to_string(uuid)).c_str());
    }

    // Drag and Drop
    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("DRAG_DROP_GAMEOBJECT", &uuid, sizeof(UID));
        ImGui::Text("Dragging %s", name.c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("DRAG_DROP_GAMEOBJECT"))
        {
            UID draggedUUID = *reinterpret_cast<const UID *>(payload->Data);
            if (draggedUUID != uuid)
            {
                if (UpdateGameObjectHierarchy(draggedUUID, uuid))
                {
                    //TODO: update the transform and AABB from all children
                }
            }
        }
        
        ImGui::EndDragDropTarget();
    }
}

void GameObject::RenderContextMenu() 
{
    if (ImGui::BeginPopup(("Game Object Context Menu##" + std::to_string(uuid)).c_str()))
    {
        if (ImGui::MenuItem("New GameObject"))
        {
            UID newUUID               = LCG().IntFast();
            GameObject *newGameObject = new GameObject(uuid, "new Game Object");
            App->GetSceneModule()->GetGameObjectByUUID(uuid)->AddGameObject(newUUID);
            App->GetSceneModule()->AddGameObject(newUUID, newGameObject);
        }

        if (uuid != App->GetSceneModule()->GetGameObjectRootUID() && ImGui::MenuItem("Delete"))
        {
            App->GetSceneModule()->RemoveGameObjectHierarchy(uuid);
        }

        ImGui::EndPopup();
    }
}

bool GameObject::UpdateGameObjectHierarchy(UID sourceUID, UID targetUID)
{
    GameObject *sourceGameObject = App->GetSceneModule()->GetGameObjectByUUID(sourceUID);
    GameObject *targetGameObject  = App->GetSceneModule()->GetGameObjectByUUID(targetUID);

    if (!sourceGameObject || !targetGameObject) return false;

    UID oldParentUUID = sourceGameObject->GetParent();
    sourceGameObject->SetParent(targetUID);

    GameObject *oldParentGameObject = App->GetSceneModule()->GetGameObjectByUUID(oldParentUUID);

    if (oldParentGameObject)
    {
        oldParentGameObject->RemoveGameObject(sourceGameObject->GetUID());
    }

    targetGameObject->AddGameObject(sourceGameObject->GetUID());

    return true;
}

void GameObject::Render()
{
    if (rootComponent != nullptr)
    {
        rootComponent->Render();
    }
}

void GameObject::RenderEditor()
{
    if (App->GetEditorUIModule()->inspectorMenu)
    {
        if (rootComponent != nullptr)
        {
            rootComponent->RenderComponentEditor();
        }
    }
    if (App->GetEditorUIModule()->hierarchyMenu)
    {
        App->GetSceneModule()->RenderHierarchyUI(App->GetEditorUIModule()->hierarchyMenu);
    }
}

void GameObject::PassAABBUpdateToParent()
{
    // TODO Update AABBs further up the gameObject tree
    globalAABB = AABB(rootComponent->GetGlobalAABB());

    for (UID child : children)
    {
        GameObject *gameObject = App->GetSceneModule()->GetGameObjectByUUID(child);

        if (gameObject != nullptr)
        {
            globalAABB.Enclose(gameObject->GetAABB());
        }
    }

    if (parentUUID != INVALID_UUID) //Filters the case of Scene GameObject (which parent is INVALID_UUID)
    {
        GameObject *parentGameObject = App->GetSceneModule()->GetGameObjectByUUID(parentUUID);

        if (parentGameObject != nullptr)
        {
            parentGameObject->PassAABBUpdateToParent();
        }
    }
}

void GameObject::ComponentGlobalTransformUpdated()
{
    if (rootComponent != nullptr)
        globalAABB = AABB(rootComponent->GetGlobalAABB());

    for (UID child : children)
    {
        GameObject *childGameObject = App->GetSceneModule()->GetGameObjectByUUID(child);

        if (childGameObject != nullptr)
        {
            globalAABB.Enclose(childGameObject->rootComponent->TransformUpdated(rootComponent == nullptr ? Transform():rootComponent->GetGlobalTransform()));
        }
    }
}

const Transform &GameObject::GetGlobalTransform() const { return rootComponent->GetGlobalTransform(); }