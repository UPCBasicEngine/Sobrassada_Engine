#include "MeshComponent.h"

#include "Application.h"
#include "CameraModule.h"
#include "EditorUIModule.h"
#include "../Root/RootComponent.h"
#include "EngineMesh.h"
#include "RenderTestModule.h"
#include "SceneModule.h"
#include "imgui.h"

#include <Math/Quat.h>

MeshComponent::MeshComponent(const UID uid, const UID uidParent, const UID uidRoot, const char* name, const Transform& parentGlobalTransform)
        : Component(uid, uidParent, uidRoot, name, parentGlobalTransform)
{
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
            ImGui::OpenPopup("CONSTANT_RESOURCE_SELECT_DIALOG_ID");
        }

        Resource* selectedResource = App->GetEditorUIModule()->RenderResourceSelectDialog(App->GetSceneModule()->MOCKUP_libraryMeshes);
        if (selectedResource != nullptr)
        {
            AddMesh(selectedResource);
        }
    
        if (ImGui::BeginPopup("MeshSelection"))
        {
            static char searchText[255] = "";
            ImGui::InputText("Search mesh", searchText, 255);
        
            ImGui::Separator();
            if (ImGui::BeginListBox("##ComponentList", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing())))
            {
                for ( const auto &modelPair : App->GetSceneModule()->MOCKUP_libraryMeshes ) {
                    {
                      if (modelPair.first.find(searchText) != std::string::npos)
                     {
                            if (ImGui::Selectable(modelPair.first.c_str(), false))
                         {
                              LoadMesh(modelPair.first, modelPair.second);
                              ImGui::CloseCurrentPopup();
                           }
                       }
                  }}
                ImGui::EndListBox();
            }
            ImGui::EndPopup();
        }

        ImGui::SeparatorText("Diffuse texture");
        ImGui::Text(currentTextureName.c_str());
        ImGui::SameLine();
        if (ImGui::Button("Select texture"))
        {
            ImGui::OpenPopup("CONSTANT_RESOURCE_SELECT_DIALOG_ID");
        }

        Resource* selectedResource = App->GetEditorUIModule()->RenderResourceSelectDialog(App->GetSceneModule()->MOCKUP_libraryTextures);
        if (selectedResource != nullptr)
        {
            AddMaterial(selectedResource);
        }
    }
    
}

void MeshComponent::RenderEditorComponentTree(const uint32_t selectedComponentUID)
{
    Component::RenderEditorComponentTree(selectedComponentUID);
}

void MeshComponent::Update()
{
}

void MeshComponent::Render(){
    if (enabled && currentMesh != CONSTANT_NO_MESH_UUID)
    {
        EngineMesh* currentMesh = App->GetSceneModule()->MOCKUP_loadedMeshes[currentMesh];
        if (currentMesh != nullptr)
        {
            float4x4 proj = App->GetCameraModule()->GetProjectionMatrix();
            float4x4 view = App->GetCameraModule()->GetViewMatrix();
            float4x4 model = float4x4::FromTRS(
                    globalTransform.position,
                    math::Quat::FromEulerXYZ(globalTransform.rotation.x, globalTransform.rotation.y, globalTransform.rotation.z),
                    globalTransform.scale);
            currentMesh->Render(App->GetRenderTestModule()->GetProgram(), App->GetSceneModule()->MOCKUP_loadedTextures[currentTexure],
                model, proj, view);
        }
    }
    Component::Render();
}

void MeshComponent::AddMesh(Resource* resource)
{
    ResourceMesh* newMesh = dynamic_cast<ResourceMesh*>(resource);
    if (newMesh != nullptr)
    {
        App->GetResourcesModule()->ReleaseResource(currentMesh);
        currentMeshName = newMesh->GetName();
        currentMesh = newMesh;
        if (mesh != nullptr)
        {
            localComponentAABB = AABB(mesh->GetAABB());
            AABBUpdatable* parent = App->GetSceneModule()->GetTargetForAABBUpdate(uidParent);
            if (parent != nullptr)
            {
                parent->PassAABBUpdateToParent();
            }
        }
    }
}

void MeshComponent::AddMaterial(Resource *resource)
{
    ResourceMaterial* newMaterial = dynamic_cast<ResourceMaterial*>(resource);
    if (newMaterial != nullptr)
    {
        App->GetResourcesModule()->ReleaseResource(currentMaterial);
        currentMaterial = newMaterial;
        currentTextureName = currentMaterial->GetName();
    }
}