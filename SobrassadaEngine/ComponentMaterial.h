#pragma once
#include "imgui.h"
#include "Math/float3.h"
#include "Math/float4.h"

class ComponentMaterial
{
    public:
        void OnEditorUpdate();

        void SetName(const std::string &newName) { name = newName; }
        void SetHasDiffuseMap(bool value) { hasDiffuseMap = value; }
        void SetDiffuseColor(const float3 &color) { diffuseColor = color; }
        void SetDiffuseMap(int map) { diffuseMap = map; }
        void SetDiffuseIntensity(float intensity) { diffuseIntensity = intensity; }

        void setHasSpecularMap(bool value) { hasSpecularMap = value; }
        void setSpecularColor(const float3& color) { specularColor = color; }
        void setSpecularMap(int map) { specularMap = map; }
        void setSpecularIntensity(float intensity) { specularIntensity = intensity; }
        void setShininess(float value) { shininess = value; }
        void setHasShininessInAlpha(bool value) { hasShininessInAlpha = value; }

        void setIsPbrSpecularGlossines(bool value) { isPbrSpecularGlossines = value; }
        void setDiffuseFactor(const float4& factor) { diffuseFactor = factor; }
        void setSpecularFactor(const float3& factor) { specularFactor = factor; }
        void setGlossinessFactor(float factor) { glossinessFactor = factor; }
        void setSpecularGlossinesTexture(int texture) { specularGlossinesTexture = texture; }
        void setSpecularGlossinessIntensity(float intensity) { specularGlossinessIntensity = intensity; }

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
