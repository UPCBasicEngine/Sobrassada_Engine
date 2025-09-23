#pragma once

#include "Globals.h"
#include "math/float4x4.h"

class GameObject;
class GBuffer;
class SSAO;
class Framebuffer;
class CameraComponent;
class DirectionalLightComponent;

constexpr int SpotLightShadowMapSize = 1024;
constexpr int TotalShadowMaps = 15;

class RenderPass
{
  public:
    RenderPass();
    ~RenderPass();

    void RenderScene(
        Framebuffer* framebuffer, const std::vector<GameObject*> objectsToRender, CameraComponent* camera,
        float deltaTime
    );

    bool IsFXAAEnabled() const { return enableFXAA; }
    bool IsShowBorders() const { return showBorders; }
    float GetGlobalThreshold() const { return globalThreshold; }
    float GetLocalThreshold() const { return localThreshold; }

    void SetEnabled(bool enable) { enableFXAA = enable; }
    void SetShowBorders(bool show) { showBorders = show; }
    void SetGlobalThreshold(float newThreshold) { globalThreshold = newThreshold; }
    void SetLocalThreshold(float newThreshold) { localThreshold = newThreshold; }

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
    void SsaoPassRender(CameraComponent* camera, GBuffer* gbuffer, SSAO* ssao) const;
    void SsaoBlurPassRender(SSAO* ssao);
    void VolumetricFogPassRender(CameraComponent* camera, DirectionalLightComponent* light);
    void AntiAliasingPassRender(Framebuffer* framebuffer) const;

    void RenderGBufferDebug(GBuffer* gbuffer) const;
    void RenderDepthDebug(GBuffer* gbuffer, CameraComponent* camera) const;
    void RenderShadowMapDebug() const;
    void RenderSsaoDebug(SSAO* ssao, CameraComponent* camera, Framebuffer* framebuffer) const;

  public:
    // Volumetric parameters
    int numStepsVolumetric        = 32;
    float fogIntensity            = 0.2f;
    float noiseAmmount            = 1.f;
    float extinctionCoefficient   = 0.04f;
    float anisotropy              = 0.5f;

  private:
    GBuffer* gbuffer         = nullptr;
    SSAO* ssao               = nullptr;
    Framebuffer* framebuffer = nullptr;
    int width, height;
    int shadowResolution = 4096;

    // Decals
    unsigned int decalVAO, decalVBO, decalEBO;

    // Shadows
    unsigned int depthTexture, depthFBO;
    float4x4 lightView;
    float4x4 lightProj;

    // SpotLight Shadows
    unsigned int spotShadowMaps[TotalShadowMaps] = {0};

    // Tile Shading
    unsigned int visibleLightIndicesSSBO = 0;
    size_t currentSize                   = 0;
    int tilesX;

    // Volumetric Fog
    unsigned int fogResultTexture = 0;

    // FXAA
    bool enableFXAA       = true;
    bool showBorders      = false;
    float globalThreshold = 0.0312f;
    float localThreshold  = 0.063f;
};