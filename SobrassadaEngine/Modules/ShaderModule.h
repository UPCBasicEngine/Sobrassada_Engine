#pragma once

#include "HashString.h"
#include "Module.h"

#include <map>

class ShaderModule : public Module
{
  public:
    ShaderModule();
    ~ShaderModule() override;

    bool Init() override;
    bool ShutDown() override;

    unsigned int SOBRASADA_API_ENGINE RequestShaderProgram(const char* vertexPath, const char* fragmentPath);
    unsigned int CreateShaderProgram(const char* vertexPath, const char* fragmentPath);
    unsigned int CreateComputeProgram(const char* computePath);
    void SOBRASADA_API_ENGINE DeleteProgram(unsigned int programID);

    int GetSpecularGlossinessProgram() const;
    int GetMetallicRoughnessProgram() const;
    int GetMetallicGeometryPassProgram() const;
    int GetMetallicGeometryVPOPassProgram() const { return metallicGeometryVPOPassProgram; }
    int GetSpecularGeometryPassProgram() const;
    int GetSpecularGeometryVPOPassProgram() const { return specularGeometryVPOPassProgram; }
    int GetLightingPassProgram() const;
    int GetTransparentPassProgram() const { return transparentPassProgram; };
    int GetTransparentVPOPassProgram() const { return transparentVPOPassProgram; }
    int GetUIWidgetProgram() const { return uiWidgetProgram; }
    int GetQuadProgram() const { return quadProgram; };
    int GetDepthProgram() const { return depthProgram; };
    int GetLinearDepthProgram() const { return linearDepthProgram; };
    int GetBillboardProgram() const { return billboardProgram; }
    int GetTrailProgram() const { return trailProgram; }
    int GetDecalProgram() const { return decalProgram; }
    int GetShadowMapPassProgram() const { return shadowMapProgram; }
    int GetComputeShadowDepthProgram() const { return shadowDepthProgram; }
    int GetTileShadingProgram() const { return tileShadingProgram; }
    int GetSpritesheetProgram() const { return spritesheetProgram; }
    int GetParticleSystemProgram() const { return particleSystemProgram; }
    int GetSsaoProgram() const { return ssaoProgram; }
    int GetSsaoDebugProgram() const { return ssaoDebugProgram; }
    int GetSsaoBlurProgram() const { return ssaoBlurProgram; }
    int GetVideoProgram() const { return videoProgram; }
    int GetVolumetricFogComputeProgram() const { return volumetricFogProgram; }

  private:
    char* LoadShaderSource(const char* shaderPath);
    unsigned int CompileShader(unsigned int shaderType, const char* source);
    unsigned int CreateProgram(unsigned int vertexShader, unsigned int fragmentShader);
    unsigned int CreateCompProgram(unsigned int computeShader);

  private:
    int specularGlossinessProgram      = -1;
    int specularGlossinessProgramUnlit = -1;

    int metallicRoughnessProgram       = -1;
    int metallicRoughnessProgramUnlit  = -1;

    int metallicGeometryPassProgram    = -1;
    int metallicGeometryVPOPassProgram = -1;
    int specularGeometryPassProgram    = -1;
    int specularGeometryVPOPassProgram = -1;
    int lightingPassProgram            = -1;

    int uiWidgetProgram                = -1;

    int transparentPassProgram         = -1;
    int transparentVPOPassProgram      = -1;
    int quadProgram                    = -1;
    int depthProgram                   = -1;
    int linearDepthProgram             = -1;
    int billboardProgram               = -1;

    int trailProgram                   = -1;
    int decalProgram                   = -1;
    int shadowMapProgram               = -1;

    int shadowDepthProgram             = -1;
    int tileShadingProgram             = -1;

    int spritesheetProgram             = -1;
    int particleSystemProgram          = -1;

    int ssaoProgram                    = -1;
    int ssaoDebugProgram               = -1;
    int ssaoBlurProgram                = -1;

    int videoProgram                   = -1;

    unsigned int volumetricFogProgram  = 0;

    std::map<HashString, unsigned int> customShaderPrograms;
};
