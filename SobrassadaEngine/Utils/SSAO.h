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
    unsigned int GetSSAOTexture() const { return ssaoTexture; }

  private:
    unsigned int ssaoFrameBufferObject = 0;
    unsigned int ssaoTexture           = 0;
    unsigned int noiseTexture          = 0;

    std::vector<float3> kernels;

    int screenHeight = 0;
    int screenWidth  = 0;
};
