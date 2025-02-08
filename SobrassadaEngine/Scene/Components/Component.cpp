#include "Component.h"

#include "Application.h"
#include "ComponentUtils.h"
#include "EditorUIModule.h"
#include "SceneModule.h"
#include "imgui.h"
#include "Root/RootComponent.h"

Component::Component(const uint32_t uuid, const uint32_t uuidParent, const uint32_t uuidRoot, const char* name, const Transform& parentGlobalTransform):
uuid(uuid), uuidParent(uuidParent), uuidRoot(uuidRoot), name(name), enabled(true), globalTransform(parentGlobalTransform){}

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

bool Component::AddComponent(const uint32_t componentUUID)
{
    children.push_back(componentUUID);
    return true;
}

bool Component::RemoveComponent(const uint32_t componentUUID)
{
    if (const auto it = std::find(children.begin(), children.end(), componentUUID); it != children.end())
    {
        children.erase(it);
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
    ImGui::Indent( 16 );

    ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (selectedComponentUUID == uuid)
    {
        base_flags |= ImGuiTreeNodeFlags_Selected;
    }
    const bool isExpanded = ImGui::TreeNodeExV((void*) uuid, base_flags, name, nullptr);
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        RootComponent* rootComponent = App->GetSceneModule()->GetRootComponent();
        if (rootComponent != nullptr)
        {
            rootComponent->SetSelectedComponent(uuid);
        }
    }
    
    if (isExpanded) 
    {
        for (uint32_t child : children)
        {
            Component* childComponent = App->GetSceneModule()->gameComponents[child];

            if (childComponent != nullptr)  childComponent->RenderEditorComponentTree(selectedComponentUUID);
        }
        ImGui::TreePop();
    }
    
    ImGui::Unindent( 16 );
}

void Component::ParentGlobalTransformUpdated(const Transform &parentGlobalTransform)
{
    globalTransform = parentGlobalTransform + localTransform;
    TransformUpdated();
}

void Component::TransformUpdated(){
    for (uint32_t child : children)
    {
        Component* childComponent = App->GetSceneModule()->gameComponents[child];

        if (childComponent != nullptr)  childComponent->ParentGlobalTransformUpdated(globalTransform);
    }
}
