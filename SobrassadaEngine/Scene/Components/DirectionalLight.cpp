#include "DirectionalLight.h"
#include "imgui.h"


DirectionalLight::DirectionalLight() : LightComponent() { 
	direction = float3(-0.2, -1.0, 0.3); 
}

DirectionalLight::~DirectionalLight() {}

void DirectionalLight::EditorParams(const int index) { 
	
	std::string title = "Directional light" + std::to_string(index);
    
	ImGui::Begin(title.c_str());
	ImGui::Text("Directional light parameters");
	ImGui::SliderFloat3("Direction ", &direction[0], -1.0, 1.0);
    ImGui::End();
}

