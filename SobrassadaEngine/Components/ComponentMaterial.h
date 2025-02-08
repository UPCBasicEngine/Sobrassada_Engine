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
	unsigned int textureID = 0;
	int width = 0;
	int height = 0;
};

struct Material
{
    float4 diffFactor;
    float4 specFactor;

    float shininess;
    int shininessInAlpha;
    int32_t padding[2] = {0, 0};
};

class ComponentMaterial
{
public:
    void OnEditorUpdate();

    void LoadMaterial(const tinygltf::Material &srcMaterial, const tinygltf::Model& sourceModel, const char* modelPath);
    void RenderMaterial(int program);

    bool GetHasDiffuseTexture() { return hasDiffuseTexture; }
    unsigned int GetDiffuseID() { return diffuseTexture.textureID; }
    bool GetHasSpecularTexture() { return hasSpecularTexture; }
    unsigned int GetSpecularID() { return specularTexture.textureID; }

private:
    std::string name;

    TextureInfo diffuseTexture;
    bool hasDiffuseTexture             = false;
    float3 diffuseColor                = {1.0f, 1.0f, 1.0f};

    TextureInfo specularTexture;
    bool hasSpecularTexture            = false;
    float3 specularColor               = {1.0f, 1.0f, 1.0f};

    float shininess                    = 300.00f;
    bool hasShininessInAlpha           = false;

    TextureInfo GetTexture(const tinygltf::Model sourceModel, int textureIndex, const char *modelPath);
};
