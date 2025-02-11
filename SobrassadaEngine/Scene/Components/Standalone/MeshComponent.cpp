#include "MeshComponent.h"

#include "Application.h"
#include "CameraModule.h"
#include "../Root/RootComponent.h"
#include "EngineMesh.h"
#include "RenderTestModule.h"
#include "SceneModule.h"
#include "imgui.h"

#include <Math/Quat.h>

MeshComponent::MeshComponent(const uint32_t uuid, const uint32_t uuidParent, const uint32_t uuidRoot, const char* name, const Transform& parentGlobalTransform)
        : Component(uuid, uuidParent, uuidRoot, name, parentGlobalTransform)
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
        ImGui::OpenPopup("MeshSelection");
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
                }
            }
            ImGui::EndListBox();
        }
        ImGui::EndPopup();
    }

    // TODO Remove duplicated code for popups
    // TODO Rework texture system (Not working properly)

    ImGui::SeparatorText("Diffuse texture");
    ImGui::Text(currentTexureName.c_str());
    ImGui::SameLine();
    if (ImGui::Button("Select texture"))
    {
        ImGui::OpenPopup("TextureSelection");
    }

    if (ImGui::BeginPopup("TextureSelection"))
    {
        static char searchText[255] = "";
        ImGui::InputText("Search texture", searchText, 255);
        
        ImGui::Separator();
        if (ImGui::BeginListBox("##ComponentList", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing())))
        {
            for ( const auto &texturePair : App->GetSceneModule()->MOCKUP_libraryTextures ) {
                {
                    if (texturePair.first.find(searchText) != std::string::npos)
                    {
                        if (ImGui::Selectable(texturePair.first.c_str(), false))
                        {
                            currentTexureUUID = texturePair.second;
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }
            }
            ImGui::EndListBox();
        }
        ImGui::EndPopup();
    }
    }
    
}

void MeshComponent::RenderEditorComponentTree(const uint32_t selectedComponentUUID)
{
    Component::RenderEditorComponentTree(selectedComponentUUID);
}

void MeshComponent::Update()
{
}

void MeshComponent::Render(){
    if (enabled && currentMeshUUID != CONSTANT_NO_MESH_UUID)
    {
        EngineMesh* currentMesh = App->GetSceneModule()->MOCKUP_loadedMeshes[currentMeshUUID];
        if (currentMesh != nullptr)
        {
            float4x4 proj = App->GetCameraModule()->GetProjectionMatrix();
            float4x4 view = App->GetCameraModule()->GetViewMatrix();
            float4x4 model = float4x4::FromTRS(
                    globalTransform.position,
                    math::Quat::FromEulerXYZ(globalTransform.rotation.x, globalTransform.rotation.y, globalTransform.rotation.z),
                    globalTransform.scale);
            currentMesh->Render(App->GetRenderTestModule()->GetProgram(), App->GetSceneModule()->MOCKUP_loadedTextures[currentTexureUUID],
                model, proj, view);
        }
    }
    Component::Render();
}

void MeshComponent::LoadMesh(const std::string& meshName, uint32_t meshUUID)
{
    currentMeshName = meshName;
    currentMeshUUID = meshUUID;
    EngineMesh* mesh = App->GetSceneModule()->MOCKUP_loadedMeshes[currentMeshUUID];
    if (mesh != nullptr)
    {
        localComponentAABB = AABB(mesh->GetAABB());
    }
}