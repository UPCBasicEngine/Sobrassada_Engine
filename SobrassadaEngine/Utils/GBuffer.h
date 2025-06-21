#pragma once

#include <vector>

class GBuffer
{
  public:
    GBuffer(int width, int height);
    ~GBuffer();

    void Bind();
    void Unbind();
    void Resize(int width, int height);
    void CheckResize();
    unsigned int GetDepthTexture() const { return depthTexture; }
    
    int GetScreenWidth() const { return screenWidth; }
    int GetScreenHeight() const { return screenHeight; }

  private:
    void InitializeGBuffer();

  public:
    unsigned int gBufferObject   = 0;

    unsigned int diffuseTexture  = 0;
    unsigned int specularTexture = 0;
    unsigned int positionTexture = 0;
    unsigned int normalTexture   = 0;

  private:
    bool shouldResize                = false;
    int screenHeight                 = 0;
    int screenWidth                  = 0;

    unsigned int depthTexture    = 0;
    unsigned int colorAttachments[4] = {};
};
