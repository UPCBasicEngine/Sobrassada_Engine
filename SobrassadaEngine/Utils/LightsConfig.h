#pragma once

#include "Globals.h"

#include "Math/float3.h"
#include "Math/float4.h"

namespace Math
{
    class float4x4;
}

class PointLight;
class SpotLight;

namespace Lights
{
    struct AmbientLightShaderData
    {
        float4 color;

        AmbientLightShaderData(const float4 &color) : color(color) {}
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
    void InitSkybox();
    void InitLightBuffers();
    void EditorParams();
    void RenderSkybox(float4x4 &projection, float4x4& view) const;
    void SetLightsShaderData();

  private:
    unsigned int LoadSkyboxTexture(const char *filename) const;
    void SetPointLightsShaderData();
    void SetSpotLightsShaderData();

  private:
    unsigned int skyboxVao;
    unsigned int skyboxTexture;
    unsigned int skyboxProgram;
    float3 ambientColor;
    float ambientIntensity;

    unsigned int ambientBufferId;
    unsigned int pointBufferId;
    unsigned int spotBufferId;

    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;
};
