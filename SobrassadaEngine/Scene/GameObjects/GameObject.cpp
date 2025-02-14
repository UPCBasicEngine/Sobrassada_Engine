#include "GameObject.h"

#include "Application.h"
#include "EditorUIModule.h"
#include "SceneModule.h"
#include "Root/RootComponent.h"

#include <Algorithm/Random/LCG.h>

GameObject::GameObject(std::string name) : name(name)
{
    CreateRootComponent();
}

GameObject::GameObject(uint32_t parentUUID, std::string name) : parentUUID(parentUUID), name(name)
{
    CreateRootComponent();
}

GameObject::~GameObject(){
    App->GetSceneModule()->gameComponents.erase(rootComponent->GetUUID());
    delete rootComponent;
    rootComponent = nullptr;
}

bool GameObject::CreateRootComponent()
{
    rootComponent = dynamic_cast<RootComponent *>(ComponentUtils::CreateEmptyComponent(COMPONENT_ROOT, LCG().IntFast(), parentUUID, -1, Transform())); // TODO Add the gameObject UUID as parent?
    // TODO Replace parentUUID above with the UUID of this gameObject
    App->GetSceneModule()->gameComponents[rootComponent->GetUUID()] = rootComponent;
    return true;
}

bool GameObject::AddGameObject(uint32_t gameObjectUUID)
{
    if (std::find(children.begin(), children.end(), gameObjectUUID) == children.end())
    {
        children.push_back(gameObjectUUID);
        return true; 
    }
    return false;
}

bool GameObject::RemoveGameObject(uint32_t gameObjectUUID)
{
    if (const auto it = std::find(children.begin(), children.end(), gameObjectUUID); it != children.end())
    {
        children.erase(it);
        return true;
    }
    return false;
}

void GameObject::OnEditor()
{
}

void GameObject::SaveToLibrary()
{
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
}

const Transform & GameObject::GetGlobalTransform() const
{
    return rootComponent->GetGlobalTransform();
}
