#pragma once
#include "imgui.h"
#include "Math/float3.h"
#include "Math/float4.h"

class ComponentMaterial
{
    public:
        void OnEditorUpdate();

        void SetName(const std::string &newName) { name = newName; }
        void SetHasDiffuseMap(const bool value) { hasDiffuseMap = value; }
        void SetDiffuseColor(const float3 &color) { diffuseColor = color; }
        void SetDiffuseMap(const int map) { diffuseMap = map; }
        void SetDiffuseIntensity(const float intensity) { diffuseIntensity = intensity; }

        void setHasSpecularMap(const bool value) { hasSpecularMap = value; }
        void setSpecularColor(const const float3& color) { specularColor = color; }
        void setSpecularMap(const int map) { specularMap = map; }
        void setSpecularIntensity(const float intensity) { specularIntensity = intensity; }
        void setShininess(const float value) { shininess = value; }
        void setHasShininessInAlpha(const bool value) { hasShininessInAlpha = value; }

        void setIsPbrSpecularGlossines(const bool value) { isPbrSpecularGlossines = value; }
        void setDiffuseFactor(const float4& factor) { diffuseFactor = factor; }
        void setSpecularFactor(const float3& factor) { specularFactor = factor; }
        void setGlossinessFactor(const float factor) { glossinessFactor = factor; }
        void setSpecularGlossinesTexture(const int texture) { specularGlossinesTexture = texture; }
        void setSpecularGlossinessIntensity(const float intensity) { specularGlossinessIntensity = intensity; }

    private:
        std::string name;

        bool hasDiffuseMap                 = false;
        float3 diffuseColor                = {0.137f, 0.263f, 0.424f};
        int diffuseMap                     = 0;
        float diffuseIntensity             = 1.0f;

        bool hasSpecularMap                = false;
        float3 specularColor               = {1.0f, 1.0f, 0.423f};
        int specularMap                    = 0;
        float specularIntensity            = 1.0f;
        float shininess                    = 300.00f;
        bool hasShininessInAlpha           = false;

        bool isPbrSpecularGlossines        = false;
        float4 diffuseFactor               = {1.0f, 1.0f, 1.0f, 1.0f};
        float3 specularFactor              = {1.0, 1.0f, 1.0f};
        float glossinessFactor             = 1.0f;
        int specularGlossinesTexture       = 0;
        float specularGlossinessIntensity  = 1.0f;
};
