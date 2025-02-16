#pragma once

#include "Math/float3.h"
#include "Math/float4.h"

#include <string>
#include <vector>

struct MaterialStruct
{

    float4 diffuseFactor          = {1.0f, 1.0f, 1.0f, 1.0f}; // RGBA
    float3 specularFactor         = {1.0f, 1.0f, 1.0f};       // RGB
    float glossinessFactor        = 1.0f;
    float occlusionStrength       = 1.0f;

    // pointers to the dds paths
    UID diffuseTexture            = 0;
    UID specularGlossinessTexture = 0;
    UID normalTexture             = 0;
    UID occlusionTexture          = 0;
};

class Material
{
  public:
    Material() = default;

    // Getters (const refs or pointers to avoid copies)
    const float4& GetDiffuseFactor() const { return diffuseFactor; }
    const float3& GetSpecularFactor() const { return specularFactor; }
    float GetGlossinessFactor() const { return glossinessFactor; }
    float GetOcclusionStrength() const { return occlusionStrength; }

    const std::string& GetDiffuseTexture() const { return diffuseTexture; }
    const std::string& GetSpecularGlossinessTexture() const { return specularGlossinessTexture; }
    const std::string& GetNormalTexture() const { return normalTexture; }
    const std::string& GetOcclusionTexture() const { return occlusionTexture; }

    // Setters (use move & pointers to avoid copies)
    void SetDiffuseFactor(const float4& newDiffuseFactor) { diffuseFactor = newDiffuseFactor; }
    void SetSpecularFactor(const float3& newSpecularFactor) { specularFactor = newSpecularFactor; }
    void SetGlossinessFactor(float newGlossiness) { glossinessFactor = newGlossiness; }
    void SetOcclusionStrength(float strength) { occlusionStrength = strength; }

    void SetSpecularGlossinessTexture(std::string texture) { specularGlossinessTexture = texture; }
    void SetNormalTexture(std::string texture) { normalTexture = texture; }
    void SetDiffuseTexture(std::string texture) { diffuseTexture = texture; }
    void SetOcclusionTexture(std::string texture) { occlusionTexture = texture; }

  private:
    float4 diffuseFactor                  = {1.0f, 1.0f, 1.0f, 1.0f}; // RGBA
    float3 specularFactor                 = {1.0f, 1.0f, 1.0f};       // RGB
    float glossinessFactor                = 1.0f;
    float occlusionStrength               = 1.0f;

    // pointers to the dds paths
    std::string diffuseTexture            = "";
    std::string specularGlossinessTexture = "";
    std::string normalTexture             = "";
    std::string occlusionTexture          = "";
};