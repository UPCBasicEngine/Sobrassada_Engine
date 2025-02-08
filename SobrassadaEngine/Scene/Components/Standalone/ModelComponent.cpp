#include "ModelComponent.h"

#include "Application.h"
#include "CameraModule.h"
#include "../Root/RootComponent.h"
#include "EngineModel.h"
#include "RenderTestModule.h"
#include "SceneModule.h"
#include "imgui.h"

#include <Algorithm/Random/LCG.h>
#include <Math/Quat.h>

ModelComponent::ModelComponent(const uint32_t uuid, const uint32_t uuidParent, const uint32_t uuidRoot, const char* name, const Transform& parentGlobalTransform)
        : Component(uuid, uuidParent, uuidRoot, name, parentGlobalTransform)
{
}

void ModelComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();
    ImGui::SeparatorText("Model");
    ImGui::Text(currentModelName.c_str());
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
            for ( const auto &modelPair : App->GetSceneModule()->MOCKUP_libraryModels ) {
                {
                    if (modelPair.first.find(searchText) != std::string::npos)
                    {
                        if (ImGui::Selectable(modelPair.first.c_str(), false))
                            
                            LoadModel(modelPair.first, modelPair.second);
                    }
                }
            }
            ImGui::EndListBox();
        }
        ImGui::EndPopup();
    }
}

void ModelComponent::RenderEditorComponentTree(const uint32_t selectedComponentUUID)
{
    Component::RenderEditorComponentTree(selectedComponentUUID);
}

void ModelComponent::Update()
{
}

void ModelComponent::Render(){
    if (currentModelUUID != CONSTANT_NO_MODEL_UUID)
    {
        EngineModel* currentModel = App->GetSceneModule()->loadedModels[currentModelUUID];
        if (currentModel != nullptr)
        {
            float4x4 proj = App->GetCameraModule()->GetProjectionMatrix();
            float4x4 view = App->GetCameraModule()->GetViewMatrix();
            float4x4 model = float4x4::FromTRS(
                    globalTransform.position,
                    math::Quat::FromEulerXYZ(globalTransform.rotation.x, globalTransform.rotation.y, globalTransform.rotation.z),
                    globalTransform.scale);
            currentModel->Render(App->GetRenderTestModule()->GetProgram(), model, proj, view);
        }
    }
    Component::Render();
}

void ModelComponent::LoadModel(const std::string& modelName, uint32_t modelUUID)
{
    currentModelName = modelName;
    currentModelUUID = modelUUID;
}