#include "LightprobeEditor.h"
#include "LightprobeManager.h"
#include "Application.h"
#include "Modules/CameraModule.h"
#include "DebugDrawModule.h"
#include "SceneModule.h"
#include "Scene.h"
#include "imgui.h"

bool LightprobeEditor::RenderEditor()
{
    if (!EngineEditorBase::RenderEditor()) return false;

    ImGui::Begin(name.c_str());

    if (!lightprobeManager)
    {
        ImGui::Text("No Lightprobe Manager available");
        ImGui::End();
        return true;
    }

    ImGui::SeparatorText("Add New Probe");
    ImGui::DragFloat3("Position", &newProbePosition.x, 0.1f);
    ImGui::DragFloat3("Size", &newProbeSize.x, 0.1f, 0.1f, 50.0f);

    ImGui::SameLine();

    if (ImGui::Button("Add at Camera"))
    {
        float3 cameraPos = App->GetCameraModule()->GetCameraPosition();
        lightprobeManager->AddProbe(cameraPos, newProbeSize);
    }
    ImGui::SameLine();
    if (ImGui::Button("Render Cubemaps"))
    {
        lightprobeManager->RenderCubemaps();
    }
    ImGui::Separator();

    RenderProbesList();
    RenderProbeProperties();
    RenderVisualization();
    
    ImGui::End();

    return true;
}

void LightprobeEditor::RenderProbesList()
{
    const auto& probes = lightprobeManager->GetProbes();
    ImGui::BeginChild("Probes List", ImVec2(0, 150), true);

    for (int i = 0; i < probes.size(); ++i)
    {
        const auto& probe  = probes[i];
        std::string label  = "Probe " + std::to_string(i);
        label             += "[" + std::to_string(probe.position.x) + " , ";
        label             += std::to_string(probe.position.y) + " , ";
        label             += std::to_string(probe.position.z) + "]";

        bool isSelected    = (selectedProbeIndex == i);

        if (ImGui::Selectable(label.c_str(), isSelected))
        {
            selectedProbeIndex = i;
        }

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete"))
            {
                lightprobeManager->RemoveProbe(i);
                if (selectedProbeIndex >= i) selectedProbeIndex--;
            }
            ImGui::EndPopup();
        }
    }
    ImGui::EndChild();
    ImGui::Text("Total probes: %d", (int)probes.size());
}

void LightprobeEditor::RenderProbeProperties()
{
    if (selectedProbeIndex < 0 || selectedProbeIndex >= lightprobeManager->GetProbes().size())
    {
        ImGui::Text("No probe selected");
        return;
    }

    ImGui::SeparatorText("Probe Properties");
    auto& probes = const_cast<std::vector<Lightprobe>&>(lightprobeManager->GetProbes());
    auto& probe  = probes[selectedProbeIndex];

    ImGui::DragFloat3("Position, ", &probe.position.x, 0.1f);
    ImGui::DragFloat3("Size, ", &probe.size.x, 0.1f, 0.1f, 50.0f);

    ImGui::Text("Needs Update: %s ", probe.needUpdate ? "Yes" : "No");
    ImGui::Text("Cubemap ID: %u", probe.cubemapTexture);
    if (probe.cubemapTexture != 0 && ImGui::Button("Preview Cubemap"))
    {
       showCubemapPreview = !showCubemapPreview;
    }

    if (showCubemapPreview && probe.cubemapTexture != 0)
    {
        ImGui::Text("Cubemap Preview (6 faces):");
        float previewSize = 64.0f;

        for (int face = 0; face < 6; ++face)
        {
            if (face > 0) ImGui::SameLine();

            (ImTextureID)(uintptr_t) probe.cubemapTexture, ImVec2(previewSize, previewSize);

            if (ImGui::IsItemHovered())
            {
                const char* faceNames[] = {"+X", "-X", "+Y", "-Y", "+Z", "-Z"};
                ImGui::SetTooltip("Face: %s", faceNames[face]);
            }
        }
    }
    ImGui::Text("Cubemap Status: %s", probe.cubemapTexture != 0 ? "Generated" : "Not Generated");
    if (probe.cubemapTexture != 0)
    {
        ImGui::Text("Texture ID: %u", probe.cubemapTexture);
    }

    if (ImGui::Button("Force Update"))
    {
        auto& probes                           = const_cast<std::vector<Lightprobe>&>(lightprobeManager->GetProbes());
        probes[selectedProbeIndex].needUpdate = true;
    }
}

void LightprobeEditor::RenderVisualization()
{
    ImGui::SeparatorText("Visualization");
    const auto& probes         = lightprobeManager->GetProbes();
    DebugDrawModule* debugDraw = App->GetDebugDrawModule();

    if (!debugDraw) return;

    for (int i = 0; i < probes.size(); ++i)
    {
        const auto probe = probes[i];
        float3 color     = (i == selectedProbeIndex) ? float3(1, 1, 0) : float3(0,1,1);

        AABB bounds      = probe.GetBounds();
        for (int j = 0; j < 12; ++j)
        {
            debugDraw->DrawLineSegment(bounds.Edge(j), color);
        }
    }
}
