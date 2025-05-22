#pragma once

#include "Globals.h"

#include "Math/float3.h"
#include "Math/float4.h"
#include "rapidjson/document.h"
#include <memory>

class Component;
class ResourceTexture;

namespace Math
{
    class float4x4;
}

namespace Lights
{
    struct AmbientLightShaderData
    {
        float4 color;
        UID cubemapIrradiance;
        UID cubemapPrefiltered;
        UID environmentBRDF;
        int numLevels;

        AmbientLightShaderData(
            const float4& color, const UID cubemapIrradiance, const UID cubemapPrefiltered, const UID cubemapBRDF,
            const int numLevels
        )
            : color(color), cubemapIrradiance(cubemapIrradiance), cubemapPrefiltered(cubemapPrefiltered),
              environmentBRDF(cubemapBRDF), numLevels(numLevels)
        {
        }
    };

    struct DirectionalLightShaderData
    {
        float4 direction;
        float4 color;

        DirectionalLightShaderData() = default;
        DirectionalLightShaderData(const float4& dir, const float4& color) : direction(dir), color(color) {}
    };

    struct PointLightShaderData
    {
        float4 position;
        float4 color;

        PointLightShaderData(const float4& pos, const float4& color) : position(pos), color(color) {}
    };

    struct SpotLightShaderData
    {
        float4 position;
        float4 color;
        float3 direction;
        float innerAngle;
        float outerAngle;

        SpotLightShaderData(
            const float4& pos, const float4& color, const float3& dir, const float inner, const float outer
        )
            : position(pos), color(color), direction(dir), innerAngle(inner), outerAngle(outer)
        {
        }
    };
} // namespace Lights

class DirectionalLightComponent;
class PointLightComponent;
class SpotLightComponent;

class LightsConfig
{
  public:
    LightsConfig();
    ~LightsConfig();

    void EditorParams(bool& lightConfig);

    void InitSkybox();
    void RenderSkybox(const float4x4& projection, const float4x4& view) const;
    unsigned int CubeMapToTexture(int width, int height);
    unsigned int PreFilteredEnvironmentMapGeneration(int width, int height);
    unsigned int EnvironmentBRDFGeneration(int width, int height);

    void InitLightBuffers();
    void SetLightsShaderData() const;
    void GetAllSceneLights();

    void AddDirectionalLight(DirectionalLightComponent* newDirectional);
    void AddPointLight(PointLightComponent* newPoint);
    void AddSpotLight(SpotLightComponent* newSpot);

    void RemoveDirectionalLight(DirectionalLightComponent* directional);
    void RemovePointLight(PointLightComponent* point);
    void RemoveSpotLight(SpotLightComponent* spot);

    void SaveData(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const;
    void LoadData(const rapidjson::Value& lights);

    void IsHDRTexture(const std::string& name);

  private:
    void LoadSkyboxTexture(UID cubemapUID);
    void FreeCubemap() const;

    void GetAllPointLights(const std::vector<Component*>& components);
    void GetAllSpotLights(const std::vector<Component*>& components);
    void GetDirectionalLight(const std::vector<Component*>& components);

    void SetDirectionalLightShaderData() const;
    void SetPointLightsShaderData() const;
    void SetSpotLightsShaderData() const;

  private:
    UID skyboxUID                          = INVALID_UID;
    unsigned int skyboxVbo                 = 0;
    unsigned int skyboxVao                 = 0;
    unsigned int skyboxID                  = 0;
    UID skyboxHandle                       = 0;
    unsigned int skyboxProgram             = 0;

    unsigned int cubemapIrradiance         = 0;
    UID irradianceHandle                   = 0;
    unsigned int prefilteredEnvironmentMap = 0;
    UID prefilteredEnvironmentMapHandle    = 0;
    unsigned int environmentBRDF           = 0;
    UID environmentBRDFHandle              = 0;

    bool isHDRTexture                      = false;

    float3 ambientColor;
    float ambientIntensity;
    unsigned int directionalBufferId            = 0;
    unsigned int ambientBufferId                = 0;
    unsigned int pointBufferId                  = 0;
    unsigned int spotBufferId                   = 0;

    int numMipMaps                              = 0;

    DirectionalLightComponent* directionalLight = nullptr;
    std::vector<PointLightComponent*> pointLights;
    std::vector<SpotLightComponent*> spotLights;

    ResourceTexture* currentTexture = nullptr;
    std::string currentTextureName  = "Not selected";

    bool firstTime                  = true;
    int irradianceMapResolution     = 512;
    int prefilteredMapResolution    = 512;
    int environmentBRDFResolution   = 512;
};