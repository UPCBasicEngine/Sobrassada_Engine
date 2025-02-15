#include "PointLight.h"

#include "DebugDrawModule.h"
#include "ImGui.h"

#include <vector>

PointLight::PointLight(UID uid, UID uidParent, UID uidRoot, const Transform &parentGlobalTransform)
    : LightComponent(uid, uidParent, uidRoot, "Point Light", COMPONENT_POINT_LIGHT, parentGlobalTransform)
{
    range      = 1;
    gizmosMode = 0;
}

// PointLight::PointLight(const float3 &position, const float range) : LightComponent()
//{
//     this->position = position;
//     this->range    = range;
//     gizmosMode     = 0;
// }

PointLight::~PointLight() {}

void PointLight::RenderEditorInspector()
{
    LightComponent::RenderEditorInspector();

    if (enabled)
    {
        ImGui::Text("Point light parameters");

        ImGui::SliderFloat3("Color", &color[0], 0.0f, 1.0f);

        ImGui::SliderFloat("Intensity", &intensity, 0.0f, 100.0f);
        ImGui::SliderFloat("Range", &range, 0.0f, 10.0f);

        ImGui::SeparatorText("Gizmos");
        ImGui::Checkbox("Draw gizmos", &drawGizmos);

        ImGui::Text("Gizmos mode");
        if (ImGui::RadioButton("Lines", &gizmosMode, 0))
            ;
        ImGui::SameLine();
        if (ImGui::RadioButton("Sphere", &gizmosMode, 1))
            ;
    }
}

void PointLight::EditorParams(int index)
{
    std::string title = "Point light " + std::to_string(index);
    ImGui::Begin(title.c_str());

    ImGui::SliderFloat3("Color", &color[0], 0.0f, 1.0f);

    ImGui::SliderFloat("Intensity", &intensity, 0.0f, 100.0f);
    ImGui::SliderFloat("Range", &range, 0.0f, 10.0f);

    ImGui::SeparatorText("Gizmos");
    ImGui::Checkbox("Draw gizmos", &drawGizmos);

    ImGui::Text("Gizmos mode");
    if (ImGui::RadioButton("Lines", &gizmosMode, 0))
        ;
    ImGui::SameLine();
    if (ImGui::RadioButton("Sphere", &gizmosMode, 1))
        ;

    ImGui::End();
}

void PointLight::Render()
{
    if (!drawGizmos) return;

    std::vector<float3> directions;
    directions.push_back(float3::unitX);
    directions.push_back(float3::unitY);
    directions.push_back(float3::unitZ);
    directions.push_back(-float3::unitX);
    directions.push_back(-float3::unitY);
    directions.push_back(-float3::unitZ);

    directions.push_back(float3(1, 0, 1).Normalized());
    directions.push_back(float3(-1, 0, 1).Normalized());
    directions.push_back(float3(-1, 0, -1).Normalized());
    directions.push_back(float3(1, 0, -1).Normalized());

    directions.push_back(float3(1, 1, 0).Normalized());
    directions.push_back(float3(-1, 1, 0).Normalized());
    directions.push_back(float3(-1, -1, 0).Normalized());
    directions.push_back(float3(1, -1, 0).Normalized());

    directions.push_back(float3(0, 1, 1).Normalized());
    directions.push_back(float3(0, 1, -1).Normalized());
    directions.push_back(float3(0, -1, -1).Normalized());
    directions.push_back(float3(0, -1, 1).Normalized());

    directions.push_back(float3(1, 1, 1).Normalized());
    directions.push_back(float3(1, -1, 1).Normalized());
    directions.push_back(float3(-1, 1, 1).Normalized());
    directions.push_back(float3(-1, -1, 1).Normalized());
    directions.push_back(float3(-1, 1, -1).Normalized());
    directions.push_back(float3(-1, -1, -1).Normalized());
    directions.push_back(float3(1, 1, -1).Normalized());
    directions.push_back(float3(1, -1, -1).Normalized());

    DebugDrawModule *debug = App->GetDebugDrawModule();

    if (gizmosMode == 0)
    {
        for (int i = 0; i < directions.size(); ++i)
        {
            debug->DrawLine(globalTransform.position, directions[i], range, float3(1, 1, 1));
        }
    }
    else
    {
        debug->DrawSphere(globalTransform.position, float3(1, 1, 1), range);
    }
}

void PointLight::DrawGizmos() const
{
    if (!drawGizmos) return;

    std::vector<float3> directions;
    directions.push_back(float3::unitX);
    directions.push_back(float3::unitY);
    directions.push_back(float3::unitZ);
    directions.push_back(-float3::unitX);
    directions.push_back(-float3::unitY);
    directions.push_back(-float3::unitZ);

    directions.push_back(float3(1, 0, 1).Normalized());
    directions.push_back(float3(-1, 0, 1).Normalized());
    directions.push_back(float3(-1, 0, -1).Normalized());
    directions.push_back(float3(1, 0, -1).Normalized());

    directions.push_back(float3(1, 1, 0).Normalized());
    directions.push_back(float3(-1, 1, 0).Normalized());
    directions.push_back(float3(-1, -1, 0).Normalized());
    directions.push_back(float3(1, -1, 0).Normalized());

    directions.push_back(float3(0, 1, 1).Normalized());
    directions.push_back(float3(0, 1, -1).Normalized());
    directions.push_back(float3(0, -1, -1).Normalized());
    directions.push_back(float3(0, -1, 1).Normalized());

    directions.push_back(float3(1, 1, 1).Normalized());
    directions.push_back(float3(1, -1, 1).Normalized());
    directions.push_back(float3(-1, 1, 1).Normalized());
    directions.push_back(float3(-1, -1, 1).Normalized());
    directions.push_back(float3(-1, 1, -1).Normalized());
    directions.push_back(float3(-1, -1, -1).Normalized());
    directions.push_back(float3(1, 1, -1).Normalized());
    directions.push_back(float3(1, -1, -1).Normalized());

    DebugDrawModule *debug = App->GetDebugDrawModule();

    if (gizmosMode == 0)
    {
        for (int i = 0; i < directions.size(); ++i)
        {
            debug->DrawLine(globalTransform.position, directions[i], range, float3(1, 1, 1));
        }
    }
    else
    {
        debug->DrawSphere(globalTransform.position, float3(1, 1, 1), range);
    }
}