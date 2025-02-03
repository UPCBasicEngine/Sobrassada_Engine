#include "ComponentMaterial.h"
#include "imgui.h"

void ComponentMaterial::OnEditorUpdate() 
{ 
	if (ImGui::CollapsingHeader("Material"))
    {
        ImGui::SliderFloat3("Diffuse Color", &diffuseColor.x, 0.0f, 1.0f);

       
        float diffuseIntensityTemp = static_cast<float>(diffuseIntensity);
        if (hasDiffuseMap &&
            ImGui::SliderFloat("Diffuse Texture Intensity", &diffuseIntensityTemp, 0.0f, 2.0f, "%f"))
            diffuseIntensity = static_cast<double>(diffuseIntensityTemp);

        ImGui::SliderFloat3("Specular Color", &specularColor.x, 0.0f, 1.0f);

      
        float specularIntensityTemp = static_cast<float>(specularIntensity);
        if (hasSpecularMap &&
            ImGui::SliderFloat("Specular Texture Intensity", &specularIntensityTemp, 0.0f, 2.0f, "%f"))
            specularIntensity = static_cast<double>(specularIntensityTemp);

       
        float shininessTemp = static_cast<float>(shininess);
        if (ImGui::SliderFloat("Shininess", &shininessTemp, 0.0f, 500.0f, "%f"))
            shininess = static_cast<double>(shininessTemp);

        
        if (isPbrSpecularGlossines)
        {
            float diffuseFactorTemp[4] = {
                static_cast<float>(diffuseFactor[0]), static_cast<float>(diffuseFactor[1]),
                static_cast<float>(diffuseFactor[2]), static_cast<float>(diffuseFactor[3])
            };
            if (ImGui::SliderFloat4("PBR Diffuse Factor", diffuseFactorTemp, 0.0f, 1.0f, "%f"))
            {
                for (int i = 0; i < 4; i++)
                {
                    diffuseFactor[i] = static_cast<double>(diffuseFactorTemp[i]);
                }
            }

            float specularFactorTemp[3] = {
                static_cast<float>(specularFactor[0]), static_cast<float>(specularFactor[1]),
                static_cast<float>(specularFactor[2])
            };
            if (ImGui::SliderFloat3("PBR Specular Factor", specularFactorTemp, 0.0f, 1.0f, "%f"))
            {
                for (int i = 0; i < 3; i++)
                {
                    specularFactor[i] = static_cast<double>(specularFactorTemp[i]);
                }
            }

            float glossinessFactorTemp = static_cast<float>(glossinessFactor);
            if (ImGui::SliderFloat("Glossiness Factor", &glossinessFactorTemp, 0.0f, 1.0f, "%f"))
                glossinessFactor = static_cast<double>(glossinessFactorTemp);

            float specularGlossinessIntensityTemp = static_cast<float>(specularGlossinessIntensity);
            if (ImGui::SliderFloat("Specular Glossiness Intensity", &specularGlossinessIntensityTemp, 0.0f, 2.0f, "%f"))
                specularGlossinessIntensity = static_cast<double>(specularGlossinessIntensityTemp);
        }
    }

}
