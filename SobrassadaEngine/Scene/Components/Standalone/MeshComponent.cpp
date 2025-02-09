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
    ImGui::SeparatorText("Model");
    ImGui::Text(currentMeshName.c_str());
    ImGui::SameLine();
    if (ImGui::Button("Select"))
    {
        ImGui::OpenPopup("ModelSelection");
    }

    if (ImGui::BeginPopup("ModelSelection"))
    {
        static char searchText[255] = "";
        ImGui::InputText("Search", searchText, 255);
        
        ImGui::Separator();
        if (ImGui::BeginListBox("##ComponentList", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing())))
        {
            for ( const auto &modelPair : App->GetSceneModule()->MOCKUP_libraryMeshes ) {
                {
                    if (modelPair.first.find(searchText) != std::string::npos)
                    {
                        if (ImGui::Selectable(modelPair.first.c_str(), false))
                            
                            LoadMesh(modelPair.first, modelPair.second);
                    }
                }
            }
            ImGui::EndListBox();
        }
        ImGui::EndPopup();
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
    if (currentMeshUUID != CONSTANT_NO_MESH_UUID)
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
            currentMesh->Render(App->GetRenderTestModule()->GetProgram(), model, proj, view);
        }
    }
    Component::Render();
}

void MeshComponent::LoadMesh(const std::string& meshName, uint32_t meshUUID)
{
    currentMeshName = meshName;
    currentMeshUUID = meshUUID;
}