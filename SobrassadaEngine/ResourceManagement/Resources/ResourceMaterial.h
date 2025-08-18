#pragma once

#include "Resource.h"
#include "Globals.h"

#include "Math/float3.h"
#include "Math/float4.h"
#include "rapidjson/document.h"

class Material;

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
    bool shininessInAlpha = false;
    float metallicFactor  = 1.0f;
    float roughnessFactor = 1.0f;
    uint64_t diffuseTex   = 0;
    uint64_t specularTex  = 0;
    uint64_t metallicTex  = 0;
    uint64_t normalTex    = 0;
    int hasSpecular       = 0;
    int hasMetallic       = 0;
    uint64_t emmisiveTex  = 0; // Right now works as padding TODO: put emmissive
    uint64_t occlusionTex = 0;
    uint64_t padding      = 0;
};

class ResourceMaterial : public Resource
{
  public:
    ResourceMaterial(UID uid, const std::string& name);
    ~ResourceMaterial() override;

    void OnEditorUpdate();
    void LoadMaterialData(const Material& mat, const rapidjson::Value& importOptions);
    void FreeMaterials() const;

    UID ChangeTexture(UID newTexture, TextureInfo& textureToChange, UID textureGPU);
    void ChangeFallBackTexture();
    void SaveToMeta();

    void SetTransparent(const bool transparent) { isTransparent = transparent; }
    void SetAlphaDiscard(const bool isAlphaDiscard) { isAlpha = isAlphaDiscard; }

    const bool GetIsSpecular() const { return specularTexture.textureID != 0 ? true : false; }
    const bool GetIsMetallicRoughness() const { return metallicTexture.textureID != 0 ? true : false; }
    const MaterialGPU GetMaterial() const { return material; }
    const bool HasNormal() const { return hasNormal; }
    const bool IsTransparent() const { return isTransparent; }
    const bool IsAlphaDiscard() const { return isAlpha; }
    const bool IsDoubleSided() const { return doubleSided; }
    bool DoApplyWind() const { return applyWind; }
    const float GetVCoord0() const { return vCoord0; }
    const float GetVCoord1() const { return vCoord1; }
    const bool UseCentralPivot() const { return useCentralPivot; }
    const bool UseWindGravity() const { return useWindGravity; }
    const float GetWindXAxis() const { return windXAxis; }
    const float GetWindYAxis() const { return windYAxis; }
    const float GetWindZAxis() const { return windZAxis; }
    const float GetWindResistance() const { return windResistance; }

    unsigned int GetDiffuseColorID() const { return diffuseTexture.textureID; }
    int GetDiffuseWidth() const { return diffuseTexture.width; }
    int GetDiffuseHeight() const { return diffuseTexture.height; }

    unsigned int GetSpecularTextureID() const { return specularTexture.textureID; }
    unsigned int GetMetallicTextureID() const { return metallicTexture.textureID; }
    unsigned int GetNormalTextureID() const { return normalTexture.textureID; }
    unsigned int GetEmissiveTextureID() const { return emmisiveTexture.textureID; }
    unsigned int GetOcclusionTextureID() const { return occlusionTexture.textureID; }

    SOBRASADA_API_ENGINE void SetDiffColor(const float4& newColor);

  private:
    TextureInfo diffuseTexture;
    TextureInfo specularTexture;
    TextureInfo metallicTexture;
    TextureInfo normalTexture;
    TextureInfo emmisiveTexture;
    TextureInfo occlusionTexture;

    MaterialGPU material  = {};
    bool isTransparent    = false;
    bool isAlpha          = false;
    bool doubleSided      = false;
    bool hasNormal        = false;
    bool applyWind        = false;
    float vCoord0 = 0.0f;
    float vCoord1 = 1.0f;
    bool useCentralPivot = false;
    bool useWindGravity          = false;
    float windXAxis = 1.0f;
    float windYAxis = 1.0f;
    float windZAxis = 1.0f;
    float windResistance = 1.0f;
    UID defaultTextureUID = INVALID_UID;

    bool wasUpdated       = false;
};
