#include "SpotLight.h"

#include "ImGui.h"

SpotLight::SpotLight() : LightComponent()
{
    position   = float3(0, 5, -2);
    direction  = -float3::unitY;
    range      = 5;
    innerAngle = 20;
    outerAngle = 60;
}

SpotLight::~SpotLight() {}

void SpotLight::EditorParams() 
{ 
    ImGui::Begin("Spot Light"); 

    ImGui::Text("Spot light parameters");

    ImGui::SliderFloat3("Position", &position[0], -10.0f, 10.0f);
    ImGui::SliderFloat3("Direction ", &direction[0], -1.0, 1.0f);
    ImGui::SliderFloat3("Color", &color[0], 0.0f, 1.0f);

    ImGui::SliderFloat("Intensity", &intensity, 0.0f, 10.0f);
    ImGui::SliderFloat("Range", &range, 0.0f, 10.0f);
    ImGui::SliderFloat("Inner angle", &innerAngle, 0.0f, 180.0f);
    ImGui::SliderFloat("Outer angle", &outerAngle, 0.0f, 180.0f);

    if (innerAngle > outerAngle) outerAngle = innerAngle;
    if (outerAngle < innerAngle) innerAngle = outerAngle;

    ImGui::End();
}