#pragma once

#include <vector>
#include "Math/float3.h"
class GBuffer;

class SSAO
{
  public:
    SSAO(int width, int height);
    ~SSAO();

    void Init();
    void Render(GBuffer& gbuffer);
    unsigned int GetSSAOTexture() const { return ssaoTexture; }

  private:
    unsigned int ssaoFBO;
    unsigned int ssaoTexture;
    unsigned int noiseTexture;

    std::vector<float3> kernel;

    int width, height;
};

