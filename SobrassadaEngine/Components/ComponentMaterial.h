#pragma once
#include "imgui.h"
#include "Math/float3.h"
#include "Math/float4.h"

class ComponentMaterial
{
    public:
        void OnEditorUpdate();

        void SetName(const std::string &newName) { name = newName; }

        void SetHasDiffuseTexture(const bool value) { hasDiffuseTexture = value; }
        void SetDiffuseColor(const float3 &color) { diffuseColor = color; }
        void SetDiffuseID(const unsigned int id) { diffuseID = id; }

        void setHasSpecularTexture(const bool value) { hasSpecularTexture = value; }
        void setSpecularColor(const const float3& color) { specularColor = color; }
        void setSpecularID(const unsigned int id) { specularID = id; }

        void setShininess(const float value) { shininess = value; }
        void setHasShininessInAlpha(const bool value) { hasShininessInAlpha = value; }

        bool GetHasDiffuseTexture() { return hasDiffuseTexture; }
        unsigned int GetDiffuseID() { return diffuseID; }
        bool GetHasSpecularTexture() { return hasSpecularTexture; }
        unsigned int GetSpecularID() { return specularID; }

    private:
        std::string name;

        bool hasDiffuseTexture             = false;
        float3 diffuseColor                = {1.0f, 1.0f, 1.0f};
        unsigned int diffuseID                      = 0;

        bool hasSpecularTexture            = false;
        float3 specularColor               = {1.0f, 1.0f, 1.0f};
        unsigned int specularID                      = 0;

        float shininess                    = 300.00f;
        bool hasShininessInAlpha           = true;
};
