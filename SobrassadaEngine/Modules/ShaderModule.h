#pragma once

#include "Module.h"

class ShaderModule : public Module
{
  public:
    ShaderModule();
    ~ShaderModule();

    unsigned int GetProgram(const char *vertexPath, const char *fragmentPath);
    void DeleteProgram(unsigned int programID);

  private:
    char *LoadShaderSource(const char *shaderPath);
    unsigned int CompileShader(unsigned int shaderType, const char *source);
    unsigned int CreateProgram(unsigned int vertexShader, unsigned fragmentShader);
};
