#include "ComponentMaterial.h"
#include "imgui.h"
void ComponentMaterial::OnEditorUpdate() 
{ 
	if (ImGui::CollapsingHeader("Material"))
    {
       
        

       
        float diffuseTemp[3] = {
            static_cast<float>(material.diffuseColor[0]), static_cast<float>(material.diffuseColor[1]),
            static_cast<float>(material.diffuseColor[2])
        };
        if (ImGui::SliderFloat3("Diffuse Color", diffuseTemp, 0.0f, 1.0f, "%f"))
        {
            material.diffuseColor[0] = static_cast<double>(diffuseTemp[0]);
            material.diffuseColor[1] = static_cast<double>(diffuseTemp[1]);
            material.diffuseColor[2] = static_cast<double>(diffuseTemp[2]);
        }

       
        float diffuseIntensityTemp = static_cast<float>(material.diffuseIntensity);
        if (material.hasDiffuseMap &&
            ImGui::SliderFloat("Diffuse Texture Intensity", &diffuseIntensityTemp, 0.0f, 2.0f, "%f"))
            material.diffuseIntensity = static_cast<double>(diffuseIntensityTemp);

       
        float specularTemp[3] = {
            static_cast<float>(material.specularColor[0]), static_cast<float>(material.specularColor[1]),
            static_cast<float>(material.specularColor[2])
        };
        if (ImGui::SliderFloat3("Specular Color", specularTemp, 0.0f, 1.0f, "%f"))
        {
            material.specularColor[0] = static_cast<double>(specularTemp[0]);
            material.specularColor[1] = static_cast<double>(specularTemp[1]);
            material.specularColor[2] = static_cast<double>(specularTemp[2]);
        }

      
        float specularIntensityTemp = static_cast<float>(material.specularIntensity);
        if (material.hasSpecularMap &&
            ImGui::SliderFloat("Specular Texture Intensity", &specularIntensityTemp, 0.0f, 2.0f, "%f"))
            material.specularIntensity = static_cast<double>(specularIntensityTemp);

       
        float shininessTemp = static_cast<float>(material.shininess);
        if (ImGui::SliderFloat("Shininess", &shininessTemp, 0.0f, 500.0f, "%f"))
            material.shininess = static_cast<double>(shininessTemp);

        
        if (material.isPbrSpecularGlossines)
        {
            float diffuseFactorTemp[4] = {
                static_cast<float>(material.diffuseFactor[0]), static_cast<float>(material.diffuseFactor[1]),
                static_cast<float>(material.diffuseFactor[2]), static_cast<float>(material.diffuseFactor[3])
            };
            if (ImGui::SliderFloat4("PBR Diffuse Factor", diffuseFactorTemp, 0.0f, 1.0f, "%f"))
            {
                for (int i = 0; i < 4; i++)
                {
                    material.diffuseFactor[i] = static_cast<double>(diffuseFactorTemp[i]);
                }
            }

            float specularFactorTemp[3] = {
                static_cast<float>(material.specularFactor[0]), static_cast<float>(material.specularFactor[1]),
                static_cast<float>(material.specularFactor[2])
            };
            if (ImGui::SliderFloat3("PBR Specular Factor", specularFactorTemp, 0.0f, 1.0f, "%f"))
            {
                for (int i = 0; i < 3; i++)
                {
                    material.specularFactor[i] = static_cast<double>(specularFactorTemp[i]);
                }
            }

            float glossinessFactorTemp = static_cast<float>(material.glossinessFactor);
            if (ImGui::SliderFloat("Glossiness Factor", &glossinessFactorTemp, 0.0f, 1.0f, "%f"))
                material.glossinessFactor = static_cast<double>(glossinessFactorTemp);

            float specularGlossinessIntensityTemp = static_cast<float>(material.specularGlossinessIntensity);
            if (ImGui::SliderFloat("Specular Glossiness Intensity", &specularGlossinessIntensityTemp, 0.0f, 2.0f, "%f"))
                material.specularGlossinessIntensity = static_cast<double>(specularGlossinessIntensityTemp);
        }
    }

}
