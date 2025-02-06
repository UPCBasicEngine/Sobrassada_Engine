#pragma once

#include "Module.h"

class Framebuffer;

class OpenGLModule : public Module
{
  public:
    OpenGLModule();
    ~OpenGLModule();

    bool Init() override;
    update_status PreUpdate(float deltaTime) override;
    update_status Update(float deltaTime) override;
    update_status PostUpdate(float deltaTime) override;
    bool ShutDown() override;

    void *GetContext() const { return context; }
    float GetClearRed() const { return clearColorRed; }
    float GetClearGreen() const { return clearColorGreen; }
    float GetClearBlue() const { return clearColorBlue; }
    Framebuffer *GetFramebuffer() const { return framebuffer; }

    void SetDepthTest(bool enable);
    void SetFaceCull(bool enable);
    void SetDepthFunc(bool enable);
    void SetFrontFaceMode(int mode);
    void SetClearRed(float newValue);
    void SetClearGreen(float newValue);
    void SetClearBlue(float newValue);

  private:
    void *context;

    Framebuffer *framebuffer = nullptr;

    float clearColorRed      = DEFAULT_GL_CLEAR_COLOR_RED;
    float clearColorGreen    = DEFAULT_GL_CLEAR_COLOR_GREEN;
    float clearColorBlue     = DEFAULT_GL_CLEAR_COLOR_BLUE;
};
