#pragma once
#include "imgui.h"
#include "Math/float3.h"
#include "Math/float4.h"
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>

class TextureModuleTest;

struct TextureInfo
{
	unsigned int textureID = -1;
	int width = 0;
	int height = 0;
};

class ComponentMaterial
{
public:
    void OnEditorUpdate();

    void LoadMaterial(const tinygltf::Material &srcMaterial, const tinygltf::Model& sourceModel, const char* modelPath);

    bool GetHasDiffuseTexture() { return hasDiffuseTexture; }
    unsigned int GetDiffuseID() { return diffuseTexture.textureID; }
    bool GetHasSpecularTexture() { return hasSpecularTexture; }
    unsigned int GetSpecularID() { return specularTexture.textureID; }
    bool GetHasNormalTexture() { return hasSpecularTexture; }
    unsigned int GetNormalID() { return normalTexture.textureID; }

private:
    std::string name;

    TextureInfo diffuseTexture;
    bool hasDiffuseTexture             = false;
    float3 diffuseColor                = {1.0f, 1.0f, 1.0f};

    TextureInfo specularTexture;
    bool hasSpecularTexture            = false;
    float3 specularColor               = {1.0f, 1.0f, 1.0f};

    TextureInfo normalTexture;
    bool hasNormalTexture            = false;

    float shininess                    = 300.00f;
    bool hasShininessInAlpha           = false;

    TextureInfo GetTexture(const tinygltf::Model sourceModel, int textureIndex, const char *modelPath);
};
