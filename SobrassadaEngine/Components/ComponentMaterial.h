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
    float4 diffColor = {1.0f, 0.0f, 0.0f, 1.0f};
    float4 specColor =  {1.0f, 0.0f, 0.0f, 1.0f};
    bool shininessInAlpha = false;
    float shininess = 500.0f;
};

class ComponentMaterial
{
public:
    void OnEditorUpdate();

    void LoadMaterial(const tinygltf::Material &srcMaterial, const tinygltf::Model& sourceModel, const char* modelPath);
    void RenderMaterial(int program);
    void FreeMaterials();
    void UpdateUBO();

private:
    std::string name;

    TextureInfo diffuseTexture;
    bool hasDiffuseTexture             = false;

    TextureInfo specularTexture;
    bool hasSpecularTexture            = false;

    Material material;

    TextureInfo GetTexture(const tinygltf::Model sourceModel, int textureIndex, const char *modelPath);
    unsigned int ubo = 0;
};
