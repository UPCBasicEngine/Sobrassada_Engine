#include "PointLight.h"

#include "DebugDrawModule.h"
#include "ImGui.h"

#include <vector>

PointLight::PointLight() : LightComponent()
{
    range      = 1;
    position   = float3(0, 0, 0);
    renderMode = 0;
}

PointLight::PointLight(const float3 &position, const float range) : LightComponent()
{
    this->position = position;
    this->range    = range;
    renderMode     = 0;
}

PointLight::~PointLight() {}

void PointLight::EditorParams(int index, bool isFirst, bool isLast)
{    
    if (isFirst)
    {
        ImGui::Begin("Point Lights");
    }
    ImGui::PushID(index);
        
    //std::string title = "Point light" + std::to_string(index);
    ImGui::Text("Point light %d", index);

    ImGui::SliderFloat3("Position", &position.x, -10.0f, 10.0f);
    ImGui::SliderFloat3("Color", &color[0], 0.0f, 1.0f);

    ImGui::SliderFloat("Intensity", &intensity, 0.0f, 100.0f);
    ImGui::SliderFloat("Range", &range, 0.0f, 10.0f);

    ImGui::Text("Gizmos mode");
    if (ImGui::RadioButton("Lines", &renderMode, 0));
    ImGui::SameLine();
    if (ImGui::RadioButton("Sphere", &renderMode, 1));

    ImGui::Separator();

    ImGui::PopID();

    if (isLast) ImGui::End();
}

void PointLight::DrawGizmos() const
{
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

    DebugDrawModule *debug = App->GetDebugDreawModule();

    if (renderMode == 0)
    {
        for (int i = 0; i < directions.size(); ++i)
        {
            debug->DrawLine(position, directions[i], range, float3(1, 1, 1));
        }
    }
    else
    {
        debug->DrawSphere(position, float3(1, 1, 1), range);
    }
}