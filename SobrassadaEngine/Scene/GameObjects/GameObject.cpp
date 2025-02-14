#include "GameObject.h"

#include "Application.h"
#include "EditorUIModule.h"
#include "Root/RootComponent.h"
#include "SceneModule.h"

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