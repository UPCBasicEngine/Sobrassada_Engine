#include "ComponentMaterial.h"
#include "imgui.h"

void ComponentMaterial::OnEditorUpdate() 
{ 
    if (ImGui::CollapsingHeader("Material"))
    {
        ImGui::SliderFloat3("Diffuse Color", &diffuseColor.x, 0.0f, 1.0f);
        if (hasDiffuseMap) ImGui::SliderFloat("Diffuse Texture Intensity", &diffuseIntensity, 0.0f, 2.0f);

        ImGui::SliderFloat3("Specular Color", &specularColor.x, 0.0f, 1.0f);
        if (hasSpecularMap) ImGui::SliderFloat("Specular Texture Intensity", &specularIntensity, 0.0f, 2.0f);
       
        ImGui::SliderFloat("Shininess", &shininess, 0.0f, 500.0f);
        
        if (isPbrSpecularGlossines)
        {
            ImGui::SliderFloat4("PBR Diffuse Factor", &diffuseFactor.x, 0.0f, 1.0f);
            ImGui::SliderFloat3("PBR Specular Factor", &specularFactor.x, 0.0f, 1.0f);

            ImGui::SliderFloat("Glossiness Factor", &glossinessFactor, 0.0f, 1.0f);
            ImGui::SliderFloat("Specular Glossiness Intensity", &specularGlossinessIntensity, 0.0f, 2.0f);
        }
    }
}
