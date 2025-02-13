#include "MeshComponent.h"

#include "Application.h"
#include "CameraModule.h"
#include "EditorUIModule.h"
#include "../Root/RootComponent.h"
#include "SceneModule.h"
#include "imgui.h"
#include "FileSystem/MeshImporter.h"

#include <Math/Quat.h>

MeshComponent::MeshComponent(const UID uid, const UID uidParent, const UID uidRoot, const Transform& parentGlobalTransform)
        : Component(uid, uidParent, uidRoot, "Mesh component", parentGlobalTransform)
{
    
}

MeshComponent::MeshComponent(const rapidjson::Value &initialState) : Component(initialState)
{
    if (initialState.HasMember("UIDMesh"))
    {
        AddMesh(initialState["UIDMesh"].GetUint64(), false); // Do not update aabb, will be done once at the end
    }
    if (initialState.HasMember("UIDMaterial"))
    {
        AddMaterial(initialState["UIDMaterial"].GetUint64());
    }
}

void MeshComponent::Save(rapidjson::Value &targetState, rapidjson::Document::AllocatorType &allocator) const
{
    Component::Save(targetState, allocator);
    targetState.AddMember("UIDMesh", currentMesh == nullptr ? CONSTANT_EMPTY_UID : currentMesh->GetUID(), allocator);
    targetState.AddMember("UIDMaterial", currentMaterial == nullptr ? CONSTANT_EMPTY_UID : currentMaterial->GetUID(), allocator);
}

void MeshComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();

    if (enabled)
    {
        ImGui::SeparatorText("Mesh");
        ImGui::Text(currentMeshName.c_str());
        ImGui::SameLine();
        if (ImGui::Button("Select mesh"))
        {
            ImGui::OpenPopup(CONSTANT_MESH_SELECT_DIALOG_ID);
        }

        if (ImGui::IsPopupOpen(CONSTANT_MESH_SELECT_DIALOG_ID))
        {
            AddMesh(App->GetEditorUIModule()->RenderResourceSelectDialog(CONSTANT_MESH_SELECT_DIALOG_ID, App->GetResourcesModule()->MOCKUP_libraryMeshes));
        }

        ImGui::SeparatorText("Diffuse texture");
        ImGui::Text(currentTextureName.c_str());
        ImGui::SameLine();
        if (ImGui::Button("Select texture"))
        {
            ImGui::OpenPopup(CONSTANT_TEXTURE_SELECT_DIALOG_ID);
        }

        if (ImGui::IsPopupOpen(CONSTANT_TEXTURE_SELECT_DIALOG_ID))
        {
            AddMaterial(App->GetEditorUIModule()->RenderResourceSelectDialog(CONSTANT_TEXTURE_SELECT_DIALOG_ID, App->GetResourcesModule()->MOCKUP_libraryTextures));
        }
    }
    
}

void MeshComponent::Update()
{
}

void MeshComponent::Render(){
    if (enabled && currentMesh != nullptr)
    {
        unsigned int cameraUBO = App->GetCameraModule()->GetUbo();
        
        float4x4 model = float4x4::FromTRS(
                globalTransform.position,
                Quat::FromEulerXYZ(globalTransform.rotation.x, globalTransform.rotation.y, globalTransform.rotation.z),
                globalTransform.scale);
        currentMesh->Render(App->GetResourcesModule()->GetProgram(), model, cameraUBO, currentMaterial);
    }
    Component::Render();
}

void MeshComponent::AddMesh(UID resource, bool reloadAABB)
{
    if (resource == CONSTANT_EMPTY_UID) return;
    
    ResourceMesh* newMesh = dynamic_cast<ResourceMesh*>(App->GetResourcesModule()->RequestResource(resource));
    if (newMesh != nullptr)
    {
        App->GetResourcesModule()->ReleaseResource(currentMesh);
        currentMeshName = newMesh->GetName();
        currentMesh = newMesh;

        if (reloadAABB)
        {
            localComponentAABB = AABB(currentMesh->GetAABB());
            AABBUpdatable* parent = App->GetSceneModule()->GetTargetForAABBUpdate(uidParent);
            if (parent != nullptr)
            {
                parent->PassAABBUpdateToParent();
            }
        }
    }
}

void MeshComponent::AddMaterial(UID resource)
{
    ResourceMaterial* newMaterial = dynamic_cast<ResourceMaterial*>(App->GetResourcesModule()->RequestResource(resource));
    if (newMaterial != nullptr)
    {
        App->GetResourcesModule()->ReleaseResource(currentMaterial);
        currentMaterial = newMaterial;
        currentTextureName = currentMaterial->GetName();
    }
}