#pragma once

#include "Globals.h"

#include "../ResourceManagement/Resources/ResourceTexture.h"

#include "Math/float3.h"
#include "Math/float4.h"

#include <memory>
#include <Libs/rapidjson/document.h>

namespace Math
{
    class float4x4;
}

class DirectionalLight;
class PointLight;
class SpotLight;

namespace Lights
{

    struct AmbientLightShaderData
    {
        float4 color;

        AmbientLightShaderData(const float4& color) : color(color) {}
    };

    struct DirectionalLightShaderData
    {
        float4 direction;
        float4 color;

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

    void AddDirectionalLight(DirectionalLight* newDirectional);
    void AddPointLight(PointLight* newPoint);
    void AddSpotLight(SpotLight* newSpot);

    void RemoveDirectionalLight();
    void RemovePointLight(UID pointUid);
    void RemoveSpotLight(UID spotUid);

    void SaveData(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const;
    void LoadData(rapidjson::Value& lights);

  private:
    unsigned int LoadSkyboxTexture(UID cubemapUID);
    void SetDirectionalLightShaderData() const;
    void SetPointLightsShaderData() const;
    void SetSpotLightsShaderData() const;

    void GetAllSceneLights();

    void GetAllPointLights();
    void GetAllSpotLights();
    void GetDirectionalLight();

  private:
    UID skyboxUID;
    unsigned int skyboxVao;
    unsigned int skyboxTexture;
    unsigned int skyboxProgram;
    float3 ambientColor;
    float ambientIntensity;
    unsigned int directionalBufferId;
    unsigned int ambientBufferId;
    unsigned int pointBufferId;
    unsigned int spotBufferId;

    DirectionalLight* directionalLight = nullptr;
    std::vector<PointLight*> pointLights;
    std::vector<SpotLight*> spotLights;

    ResourceTexture* currentTexture;
    std::string currentTextureName;
};