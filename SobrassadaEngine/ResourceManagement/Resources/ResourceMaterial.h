#pragma once
#include "Resource.h"

#include <Math/float4.h>

namespace tinygltf {class Model;
struct Material;}struct TextureInfo
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
    int padding[2] = {0, 0};
};

class ResourceMaterial : public Resource
{
public:

    ResourceMaterial(UID uid, const std::string & name);

    ~ResourceMaterial() override;
    
    void OnEditorUpdate();

    void LoadMaterial(const tinygltf::Material &srcMaterial, const tinygltf::Model& sourceModel, const char* modelPath);
    void RenderMaterial(int program);
    void FreeMaterials();
    void UpdateUBO();

private:

    TextureInfo diffuseTexture;
    bool hasDiffuseTexture             = false;

    TextureInfo specularTexture;
    bool hasSpecularTexture            = false;

    Material material;

    TextureInfo GetTexture(const tinygltf::Model sourceModel, int textureIndex, const char *modelPath);
    unsigned int ubo = 0;
};

