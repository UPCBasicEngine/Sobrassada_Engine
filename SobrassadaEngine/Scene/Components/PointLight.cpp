#include "PointLight.h"

#include "DebugDrawModule.h"
#include "ImGui.h"

#include <vector>

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
   for (int i = 0; i < directions.size(); ++i)
   {
       debug->Render2DLine(position, directions[i], range, float3(1, 1, 1));
   }
}