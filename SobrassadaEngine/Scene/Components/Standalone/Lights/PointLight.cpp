#include "PointLight.h"

#include "Application.h"
#include "SceneModule.h"

#include "DebugDrawModule.h"
#include "ImGui.h"

#include <vector>

PointLight::PointLight(UID uid, UID uidParent, UID uidRoot, const Transform& parentGlobalTransform)
    : LightComponent(uid, uidParent, uidRoot, "Point Light", COMPONENT_POINT_LIGHT, parentGlobalTransform)
{
    range      = 1;
    gizmosMode = 0;

    LightsConfig* lightsConfig = App->GetSceneModule()->GetLightsConfig();
    if (lightsConfig != nullptr) lightsConfig->AddPointLight(this);
}

PointLight::PointLight(const rapidjson::Value& initialState) : LightComponent(initialState)
{

    if (initialState.HasMember("Range"))
    {
        range = initialState["Range"].GetFloat();
    }
    if (initialState.HasMember("GizmosMode"))
    {
        gizmosMode = initialState["GizmosMode"].GetInt();
    }
    LightsConfig* lightsConfig = App->GetSceneModule()->GetLightsConfig();
    if (lightsConfig != nullptr) lightsConfig->AddPointLight(this);
}


PointLight::~PointLight() {
    App->GetSceneModule()->GetLightsConfig()->RemovePointLight(uid);
}

void PointLight::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    LightComponent::Save(targetState, allocator);

    targetState.AddMember("Range", range, allocator);
    targetState.AddMember("GizmosMode", gizmosMode, allocator);
}

void PointLight::RenderEditorInspector()
{
    LightComponent::RenderEditorInspector();

    if (enabled)
    {
        ImGui::Text("Point light parameters");
        ImGui::SliderFloat("Range", &range, 0.0f, 10.0f);

        ImGui::SeparatorText("Gizmos");
        ImGui::Checkbox("Draw gizmos", &drawGizmos);

        ImGui::Text("Gizmos mode");
        if (ImGui::RadioButton("Lines", &gizmosMode, 0))
        {
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Sphere", &gizmosMode, 1))
        {
        }
    }
}

void PointLight::Render()
{
    if (!enabled || !drawGizmos) return;

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

    DebugDrawModule* debug = App->GetDebugDrawModule();

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