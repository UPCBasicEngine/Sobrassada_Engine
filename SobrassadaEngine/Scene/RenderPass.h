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
    void GeometryPassRender(const std::vector<GameObject*>& objectsToRender, CameraComponent* camera, GBuffer* gbuffer) const;
    void NavMeshPassRender(const std::vector<GameObject*>& objectsToRender, CameraComponent* camera, GBuffer* gbuffer) const;
    void ShadowMapPassRender(
        CameraComponent* camera, DirectionalLightComponent* light, const std::vector<GameObject*>& objectsToRender
    );
    void DecalsPassRender(const std::vector<GameObject*>& objectsToRender, CameraComponent* camera, GBuffer* gbuffer) const;
    void LightingPassRender(CameraComponent* camera, GBuffer* gbuffer, Framebuffer* framebuffer) const;
    void TransparentPassRender(
        const std::vector<GameObject*>& objectsToRender, CameraComponent* camera, Framebuffer* framebuffer
    ) const;

    void RenderGBufferDebug(GBuffer* gbuffer, Framebuffer* framebuffer) const;
    void RenderDepthDebug(GBuffer* gbuffer, CameraComponent* camera, Framebuffer* framebuffer) const;
    void RenderShadowMapDebug(Framebuffer* framebuffer) const;

  private:
    GBuffer* gbuffer = nullptr;

    unsigned int decalVAO, decalVBO, decalEBO;
    unsigned int depthTexture, depthFBO;
    float4x4 lightview;
    float4x4 lightProj;
};