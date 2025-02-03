#pragma once
#include "imgui.h"
#include "Math/float3.h"
#include "Math/float4.h"

class ComponentMaterial
{
    public:
        void OnEditorUpdate();

    private:
        std::string name;

        bool hasDiffuseMap                 = false;
        float3 diffuseColor                = {0.0f, 0.0f, 0.0f};
        int diffuseMap                     = 0;
        float diffuseIntensity             = 1.0f;

        bool hasSpecularMap                = false;
        float3 specularColor               = {0.0f, 0.0f, 0.0f};
        int specularMap                    = 0;
        float specularIntensity            = 1.0f;
        float shininess                    = 100.00f;
        bool hasShininessInAlpha           = false;

        bool isPbrSpecularGlossines        = false;
        float4 diffuseFactor               = {1.0f, 1.0f, 1.0f, 1.0f};
        float3 specularFactor              = {1.0, 1.0f, 1.0f};
        double glossinessFactor            = 1.0f;
        int specularGlossinesTexture       = 0;
        float specularGlossinessIntensity  = 1.0f;
};
