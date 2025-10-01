#pragma once

#include "Globals.h"
#include "math/float2.h"
#include "math/float4x4.h"

#include "rapidjson/document.h"

class GameObject;
class GBuffer;
class SSAO;
class Framebuffer;
class CameraComponent;
class DirectionalLightComponent;
class ResourceTexture;

constexpr int SpotLightShadowMapSize = 1024;
constexpr int TotalShadowMaps        = 15;

struct SpotlightShadow
{
    float4x4 viewProjection;
    uint64_t shadowMap;
    float2 padding;
};
class BatchManager;

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

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const;
    void LoadData(const rapidjson::Value& initialState);

    void UpdateVolumetricNoiseTexture(UID newTextureUID);
    void RemoveVolumetricNoiseTexture();
    void Resize(int width, int height) const;

    bool IsFXAAEnabled() const { return enableFXAA; }
    bool IsShowBorders() const { return showBorders; }
    float GetGlobalThreshold() const { return globalThreshold; }
    float GetLocalThreshold() const { return localThreshold; }
    const ResourceTexture* GetResourceTexture() const { return noiseTexture; }
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
    void VolumetricFogPassRender(CameraComponent* camera, DirectionalLightComponent* light);
    void HeightFogPassRender(CameraComponent* camera) const;
    void AntiAliasingPassRender(Framebuffer* framebuffer) const;

    void RenderGBufferDebug(GBuffer* gbuffer) const;
    void RenderDepthDebug(GBuffer* gbuffer, CameraComponent* camera) const;
    void RenderShadowMapDebug() const;
    void RenderSsaoDebug(SSAO* ssao, CameraComponent* camera, Framebuffer* framebuffer) const;

  public:
    // Volumetric parameters
    float stepSize              = 0.5f;
    float fogIntensity          = 1.f;
    float noiseAmmount          = 0.f;
    float extinctionCoefficient = 0.04f;
    float anisotropy            = 0.5f;
    bool useNoiseTexture        = false;

  private:
    GBuffer* gbuffer         = nullptr;
    SSAO* ssao               = nullptr;
    Framebuffer* framebuffer = nullptr;
    BatchManager* batchManager = nullptr;
    int width, height;
    int shadowResolution = 4096;

    // Decals
    unsigned int decalVAO, decalVBO, decalEBO;

    // Shadows
    unsigned int depthTexture, depthFBO, spotShadowSSBO;
    float4x4 lightView;
    float4x4 lightProj;

    // SpotLight Shadows
    unsigned int spotShadowMaps[TotalShadowMaps]    = {0};
    unsigned int spotShadowMapsGPU[TotalShadowMaps] = {0};

    // Tile Shading
    unsigned int visibleLightIndicesSSBO            = 0;
    size_t currentSize                              = 0;
    int tilesX;

    // Volumetric Fog
    unsigned int fogResultTexture                 = 0;
    unsigned int visibleVolumetricAreaIndicesSSBO = 0;
    ResourceTexture* noiseTexture                 = nullptr;

    // FXAA
    bool enableFXAA                               = true;
    bool showBorders                              = false;
    float globalThreshold                         = 0.0312f;
    float localThreshold                          = 0.063f;
    HeightFogParameters heightFog;
    FXAAParameters fxaaParameters;
};