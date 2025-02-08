#include "PointLight.h"

#include "ImGui.h"

PointLight::PointLight() : LightComponent()
{
    range    = 2;
    position = float3(0, 0, 1);
}

PointLight::~PointLight() {}

void PointLight::EditorParams()
{
    ImGui::Begin("Point light");

    ImGui::Text("Point light parameters");

    ImGui::SliderFloat3("Position", &position.x, -10.0f, 10.0f);
    ImGui::SliderFloat3("Color", &color[0], 0.0f, 1.0f);

    ImGui::SliderFloat("Intensity", &intensity, 0.0f, 10.0f);
    ImGui::SliderFloat("Range", &range, 0.0f, 10.0f);
 
    ImGui::End();
}