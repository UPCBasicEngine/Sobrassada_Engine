#pragma once

#include <memory>

class Framebuffer
{
  public:
    Framebuffer(int witdh, int height, bool useRbo);
    ~Framebuffer();

    void Bind();
    void Unbind();
    void Resize(int width, int height);
    void CheckResize();

    unsigned int GetFramebufferID() const { return fbo; };
    unsigned int GetTextureID() const { return framebufferTexture; }
    unsigned int GetTextureWidth() const { return textureWidth; }
    unsigned int GetTextureHeight() const { return textureHeight; }

  private:
    bool Initialize();
    void CreateTexture(int width, int height);

  private:
    unsigned int fbo = 0, rbo = 0;
    unsigned int framebufferTexture = 0;
    int textureWidth = 0, textureHeight = 0;
    bool shouldResize = false, useRbo = false;
};
