#pragma once

#include "Globals.h"
#include "math/float4x4.h"

class GameObject;
class GBuffer;
class Framebuffer;
class CameraComponent;
class DirectionalLightComponent;

class RenderPass
{
  public:
    RenderPass();
    ~RenderPass();

    void RenderScene(Framebuffer* framebuffer, const std::vector<GameObject*> objectsToRender, CameraComponent* camera);

  private:
    void Bind() const;
    void CopyDepth() const;
    void CopyDepthStencil() const;

    void GeometryPassRender(const std::vector<GameObject*>& objectsToRender, CameraComponent* camera) const;
    void NavMeshPassRender(const std::vector<GameObject*>& objectsToRender, CameraComponent* camera) const;
    void ShadowMapPassRender(
        CameraComponent* camera, DirectionalLightComponent* light, const std::vector<GameObject*>& objectsToRender
    );
    void DecalsPassRender(const std::vector<GameObject*>& objectsToRender, CameraComponent* camera) const;
    void TileShadingPass(CameraComponent* camera, GBuffer* gbuffer, Framebuffer* framebuffer);
    void LightingPassRender(CameraComponent* camera, GBuffer* gbuffer, Framebuffer* framebuffer) const;
    void TransparentPassRender(const std::vector<GameObject*>& objectsToRender, CameraComponent* camera) const;

    void RenderGBufferDebug(GBuffer* gbuffer) const;
    void RenderDepthDebug(GBuffer* gbuffer, CameraComponent* camera) const;
    void RenderShadowMapDebug() const;

  private:
    GBuffer* gbuffer         = nullptr;
    Framebuffer* framebuffer = nullptr;
    int width, height;
    int shadowResolution = 4096;

    //Decals
    unsigned int decalVAO, decalVBO, decalEBO;

    //Shadows
    unsigned int depthTexture, depthFBO;
    float4x4 lightView;
    float4x4 lightProj;

    // Tile Shading
    unsigned int visibleLightIndicesSSBO = 0;
    size_t currentSize = 0;
};