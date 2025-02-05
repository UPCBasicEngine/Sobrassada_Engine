#include "ComponentMaterial.h"
#include "imgui.h"

void ComponentMaterial::OnEditorUpdate() 
{ 
    if (ImGui::CollapsingHeader("Material"))
    {
        ImGui::SliderFloat3("Diffuse Color", &diffuseColor.x, 0.0f, 1.0f);
        ImGui::SliderFloat3("Specular Color", &specularColor.x, 0.0f, 1.0f);
        ImGui::SliderFloat("Shininess", &shininess, 0.0f, 500.0f);
    }
}
