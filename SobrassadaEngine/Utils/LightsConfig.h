#pragma once

#include "Globals.h"

#include "Math/float3.h"
#include "Math/float4.h"

namespace Math
{
class float4x4;
}

class DirectionalLight;

namespace Lights
{

struct DirectionalLightShaderData
{
    float3 direction;
    float4 color;

    DirectionalLightShaderData(const float3 &dir, const float4 &color) : direction(dir) ,color(color) {}

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
    void RenderSkybox(float4x4 &projection, float4x4 &view) const;

    void InitLightBuffers();
    void SetLightsShaderData() const;
    void AddDirectionalLight();
    void RemoveDirectionalLight();

  private:
    unsigned int LoadSkyboxTexture(const char *filename) const;
    void SetDirectionalLightShaderData() const;


  private:
    unsigned int skyboxVao;
    unsigned int skyboxTexture;
    unsigned int skyboxProgram;
    float3 ambientColor;
    float ambientIntensity;

    unsigned int directionalBufferId;

    DirectionalLight *directionalLight = nullptr;
};