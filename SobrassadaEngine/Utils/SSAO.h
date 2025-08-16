#pragma once

#include "Math/float3.h"
#include <vector>
class GBuffer;

class SSAO
{
  public:
    SSAO(int width, int height);
    ~SSAO();

    void Init();
    void Bind();
    void Unbind();
    void Resize(int width, int height);
    void CheckResize();

    unsigned int GetSSAOTexture() const { return ssaoTexture; }
    unsigned int GetNoiseTexture() const { return noiseTexture; }
    unsigned int GetBlurTexture(int index) const { return blurTexture[index]; }
    unsigned int GetBlurFBO(int index) const { return ssaoBlurFrameBufferObject[index]; }
    const std::vector<float3>& GetKernels() const { return kernels; }
    const std::vector<float3>& GetNoise() const { return noise; }

    int GetWidth() const { return screenWidth; }
    int GetHeight() const { return screenHeight; }

  private:
    unsigned int ssaoFrameBufferObject        = 0;
    unsigned int ssaoBlurFrameBufferObject[2] = {0, 0};
    unsigned int blurTexture[2]               = {0, 0};
    unsigned int ssaoTexture                  = 0;
    unsigned int noiseTexture                 = 0;

    std::vector<float3> noise;
    std::vector<float3> kernels;

    int screenHeight  = 0;
    int screenWidth   = 0;
    bool shouldResize = false;
};
