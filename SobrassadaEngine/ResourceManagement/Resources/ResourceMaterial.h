#pragma once
#include "Resource.h"

#include <Math/float4.h>
#include <Material.h>

namespace tinygltf
{
    class Model;
    struct Material;
} // namespace tinygltf
struct TextureInfo
{
    unsigned int textureID = 0;
    int width              = 0;
    int height             = 0;
};

struct MaterialGPU
{
    float4 diffColor      = {1.0f, 0.0f, 0.0f, 1.0f};
    float3 specColor      = {1.0f, 0.0f, 0.0f};
    float shininess       = 500.0f;
    int shininessInAlpha  = 0;
    int hasNormal         = 0;
    int padding2[2]        = {0, 0};
};

class ResourceMaterial : public Resource
{
  public:
    ResourceMaterial(UID uid, const std::string& name);

    ~ResourceMaterial() override;

    void OnEditorUpdate();

    void LoadMaterialData(Material mat);
    void RenderMaterial(int program) const;
    void FreeMaterials() const;
    void UpdateUBO() const;

  private:
    TextureInfo diffuseTexture;
    bool hasDiffuseTexture = false;

    TextureInfo specularTexture;
    bool hasSpecularTexture = false;

    TextureInfo normalTexture;
    bool hasNormalTexture = false;

    MaterialGPU material;

    unsigned int ubo = 0;
};
