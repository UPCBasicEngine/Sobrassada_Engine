#pragma once

#include "Globals.h"

#include "Math/float3.h"

namespace Math
{
    class float4x4;
}

class LightsConfig
{
  public:
    LightsConfig();
    ~LightsConfig();

  public:
    void InitSkybox();
    void RenderSkybox(float4x4 &projection, float4x4& view) const;

  private:
    unsigned int LoadSkyboxTexture(const char *filename) const;

  private:
    unsigned int skyboxVao;
    unsigned int skyboxTexture;
    unsigned int skyboxProgram;
    float3 ambientColor;
    float ambientIntensity;
};
