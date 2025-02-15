#include "DirectionalLight.h"
#include "imgui.h"


DirectionalLight::DirectionalLight(UID uid, UID uidParent, UID uidRoot, const Transform &parentGlobalTransform)
    : LightComponent(uid, uidParent, uidRoot, "Directional Light", COMPONENT_DIRECTIONAL_LIGHT, parentGlobalTransform)
{
	//direction = float3(-0.2, -1.0, 0.3); 
}

DirectionalLight::~DirectionalLight() {}

void DirectionalLight::RenderEditorInspector()
{
    LightComponent::RenderEditorInspector();

    if (enabled)
    {
        ImGui::Text("Directional light parameters");
        ImGui::SliderFloat3("Direction ", &direction[0], -1.0, 1.0);
    }
}

void DirectionalLight::EditorParams(const int index) { 
	
	std::string title = "Directional light" + std::to_string(index);
    
	ImGui::Begin(title.c_str());
	ImGui::Text("Directional light parameters");
	ImGui::SliderFloat3("Direction ", &direction[0], -1.0, 1.0);
    ImGui::End();
}

