#include "ComponentMaterial.h"
#include "imgui.h"

void ComponentMaterial::OnEditorUpdate() 
{ 
    if (ImGui::CollapsingHeader("Material"))
    {
        if (!hasDiffuseTexture) ImGui::SliderFloat3("Diffuse Color", &diffuseColor.x, 0.0f, 1.0f);
        if (!hasSpecularTexture) ImGui::SliderFloat3("Specular Color", &specularColor.x, 0.0f, 1.0f);
        if (!hasShininessInAlpha) ImGui::SliderFloat("Shininess", &shininess, 0.0f, 500.0f);
    }
}
