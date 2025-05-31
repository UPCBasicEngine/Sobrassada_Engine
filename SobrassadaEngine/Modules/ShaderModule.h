#pragma once

#include "Module.h"

class ShaderModule : public Module
{
  public:
    ShaderModule();
    ~ShaderModule() override;

    bool Init() override;
    bool ShutDown() override;

    unsigned int CreateShaderProgram(const char* vertexPath, const char* fragmentPath);
    void DeleteProgram(unsigned int programID);

    int GetSpecularGlossinessProgram() const;
    int GetMetallicRoughnessProgram() const;
    int GetMetallicGeometryPassProgram() const;
    int GetSpecularGeometryPassProgram() const;
    int GetLightingPassProgram() const;
    int GetTransparentPassProgram() const { return transparentPassProgram; };
    int GetUIWidgetProgram() const { return uiWidgetProgram; }
    int GetQuadProgram() const { return quadProgram; };
    int GetDepthProgram() const { return depthProgram; };
    int GetBillboardProgram() const { return billboardProgram; }
    int GetTrailProgram() const { return trailProgram; }
    int GetDecalProgram() const { return decalProgram; }

  private:
    char* LoadShaderSource(const char* shaderPath);
    unsigned int CompileShader(unsigned int shaderType, const char* source);
    unsigned int CreateProgram(unsigned int vertexShader, unsigned fragmentShader);

  private:
    int specularGlossinessProgram      = -1;
    int specularGlossinessProgramUnlit = -1;

    int metallicRoughnessProgram       = -1;
    int metallicRoughnessProgramUnlit  = -1;

    int metallicGeometryPassProgram    = -1;
    int specularGeometryPassProgram    = -1;
    int lightingPassProgram            = -1;

    int uiWidgetProgram                = -1;

    int transparentPassProgram         = -1;
    int quadProgram                    = -1;
    int depthProgram                   = -1;
    int billboardProgram               = -1;

    int trailProgram                   = -1;
    int decalProgram                   = -1;
};
