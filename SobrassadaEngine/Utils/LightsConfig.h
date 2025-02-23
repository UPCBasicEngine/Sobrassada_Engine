#pragma once

#include "Globals.h"

#include "../ResourceManagement/Resources/ResourceTexture.h"

#include "Math/float3.h"
#include "Math/float4.h"

#include <memory>

namespace Math
{
class float4x4;
}

class DirectionalLightComponent;
class PointLightComponent;
class SpotLightComponent;

namespace Lights
{

    struct AmbientLightShaderData
    {
        float4 color;

        AmbientLightShaderData(const float4 &color) : color(color) {}
    };

    struct DirectionalLightShaderData
    {
        float4 direction;
        float4 color;

        DirectionalLightShaderData(const float4 &dir, const float4 &color) : direction(dir) ,color(color) {}

    };

    struct PointLightShaderData
    {
        float4 position;
        float4 color;

        PointLightShaderData(const float4 &pos, const float4 &color) : position(pos), color(color) {}
    };

    struct SpotLightShaderData
    {
        float4 position;
        float4 color;
        float3 direction;
        float innerAngle;
        float outerAngle;

        SpotLightShaderData(const float4 &pos, const float4 &color, const float3 &dir, const float inner, const float outer)
            : position(pos), color(color), direction(dir), innerAngle(inner), outerAngle(outer)
        {
        }
    };
} // namespace Lights

class LightsConfig
{
  public:
    LightsConfig();
    ~LightsConfig();

  public:
    void EditorParams();

    void InitSkybox();
    void RenderSkybox() const;

    void AddSkyboxTexture(UID resource);

    void InitLightBuffers();
    void RenderLights() const;

    void AddDirectionalLight(DirectionalLightComponent* newDirectional);
    void AddPointLight(PointLightComponent* newPoint);
    void AddSpotLight(SpotLightComponent* newSpot);

    void RemoveDirectionalLight();
    void RemovePointLight(UID pointUid);
    void RemoveSpotLight(UID spotUid);

  private:
    unsigned int LoadSkyboxTexture(UID cubemapUID) const;
    void SetDirectionalLightShaderData() const;
    void SetPointLightsShaderData() const;
    void SetSpotLightsShaderData() const;

    void GetAllSceneLights();

    void GetAllPointLights();
    void GetAllSpotLights();
    void GetDirectionalLight();

  private:
    unsigned int skyboxVao;
    unsigned int skyboxTexture;
    unsigned int skyboxProgram;
    float3 ambientColor;
    float ambientIntensity;
    unsigned int directionalBufferId;
    unsigned int ambientBufferId;
    unsigned int pointBufferId;
    unsigned int spotBufferId;

    DirectionalLightComponent *directionalLight = nullptr;
    std::vector<PointLightComponent*> pointLights;
    std::vector<SpotLightComponent*> spotLights;

    ResourceTexture* currentTexture;
    std::string currentTextureName;
};