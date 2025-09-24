#pragma once

#include "Globals.h"
#include "math/float4x4.h"

class GameObject;
class GBuffer;
class SSAO;
class Framebuffer;
class CameraComponent;
class DirectionalLightComponent;

struct HeightFogParameters
{
    bool isEnabled        = false;
    bool followCamera     = false;
    float densityConstant = 1.0f;
    float heightFalloff   = 1.0f;
    float maxFog          = 1.0f;
    float fogStartHeight  = 0.0f;
    float3 fogColor       = float3::one;
};

struct FXAAParameters
{
    bool isEnabled        = true;
    bool showBorders      = false;
    float globalThreshold = 0.0312f;
    float localThreshold  = 0.16f;
};

class RenderPass
{
  public:
    RenderPass();
    ~RenderPass();

    void RenderScene(
        Framebuffer* framebuffer, const std::vector<GameObject*> objectsToRender, CameraComponent* camera,
        float deltaTime
    );

    HeightFogParameters GetHeightFogParameters() const { return heightFog; }
    void SetHeightFogParameters(const HeightFogParameters& params) { heightFog = params; }

    FXAAParameters GetFXAAParameters() const { return fxaaParameters; }
    void SetFXAAParameters(const FXAAParameters& params) { fxaaParameters = params; }

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
    void HeightFogPassRender(CameraComponent* camera) const;
    void AntiAliasingPassRender(Framebuffer* framebuffer) const;

    void RenderGBufferDebug(GBuffer* gbuffer) const;
    void RenderDepthDebug(GBuffer* gbuffer, CameraComponent* camera) const;
    void RenderShadowMapDebug() const;
    void RenderSsaoDebug(SSAO* ssao, CameraComponent* camera, Framebuffer* framebuffer) const;

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

    // Tile Shading
    unsigned int visibleLightIndicesSSBO = 0;
    size_t currentSize                   = 0;
    int tilesX;

    HeightFogParameters heightFog;
    FXAAParameters fxaaParameters;
};