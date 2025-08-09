#pragma once

#include "Module.h"

class Framebuffer;
class GBuffer;
class SSAO;
typedef unsigned int GLenum;
typedef int GLsizei;
typedef int GLint;

class OpenGLModule : public Module
{
  public:
    OpenGLModule();
    ~OpenGLModule() override;

    bool Init() override;
    update_status PreUpdate(float deltaTime) override;
    update_status Update(float deltaTime) override;
    update_status PostUpdate(float deltaTime) override;
    bool ShutDown() override;

    SOBRASADA_API_ENGINE void DrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices);
    void DrawArrays(GLenum mode, GLint first, GLsizei count);

    void AddTrianglesPerSecond(float meshTrianglesPerSecond) { trianglesPerSecond += meshTrianglesPerSecond; }
    void AddVerticesCount(int meshVertices) { verticesCount += meshVertices; }
    void AddDrawCallsCount() { drawCallsCount += 1; }

    void* GetContext() const { return context; }
    float GetClearRed() const { return clearColorRed; }
    float GetClearGreen() const { return clearColorGreen; }
    float GetClearBlue() const { return clearColorBlue; }
    Framebuffer* GetFramebuffer() const { return framebuffer; }
    GBuffer* GetGBuffer() const { return gBuffer; }
    SSAO* GetSsao() const { return ssao; }
    int GetDrawCallsCount() const { return drawCallsCount; }
    float GetTrianglesPerSecond() const { return trianglesPerSecond; }
    int GetVerticesCount() const { return verticesCount; }

    void SetDepthTest(bool enable);
    void SetFaceCull(bool enable);
    void SetDepthFunc(bool enable);
    void SetFrontFaceMode(int mode);
    void SetClearRed(float newValue) { clearColorRed = newValue; }
    void SetClearGreen(float newValue) { clearColorGreen = newValue; }
    void SetClearBlue(float newValue) { clearColorBlue = newValue; }
    void SetRenderWireframe(bool renderWireframe);

  private:
    void* context            = nullptr;
    Framebuffer* framebuffer = nullptr;
    GBuffer* gBuffer         = nullptr;
    SSAO* ssao               = nullptr;
    float clearColorRed      = DEFAULT_GL_CLEAR_COLOR_RED;
    float clearColorGreen    = DEFAULT_GL_CLEAR_COLOR_GREEN;
    float clearColorBlue     = DEFAULT_GL_CLEAR_COLOR_BLUE;
    int drawCallsCount       = 0;
    float trianglesPerSecond = 0;
    int verticesCount        = 0;
};
