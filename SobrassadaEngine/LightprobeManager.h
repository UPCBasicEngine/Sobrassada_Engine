#pragma once
#include "Lightprobe.h"
#include "Math/float4x4.h"
#include "Math/float4.h"
#include "Math/float3.h"
#include <vector>

struct GPULightprobeData
{
    float4 positionAndInfluence; // xyz = position, w = influence
    float4 boundsMin;            // xyz = boundsMin, w = fadeDistance
    float4 boundsMax;            // xyz = boundsMax, w = cubemapIndex
    float4 proxyMin;             // xyz = proxyMin, w = padding
    float4 proxyMax;             // xyz = proxyMax, w = padding
};
class CameraComponent;
class LightprobeManager
{
  public:
    LightprobeManager() = default;
    ~LightprobeManager();

    void AddProbe(const float3& position, const float3& size);
    void RemoveProbe(int index);
    void RenderCubemaps();
    bool IsRenderingCubemaps() const { return isRenderingCubemaps; }

    const std::vector<Lightprobe>& GetProbes() const { return probes; }

    void MarkSceneChanged() { sceneChanged = true; }
    std::vector<GPULightprobeData> GetRelevantLightprobes(const float3& worldPos, int maxProbes = 8);
    void BindLightprobesToShader(unsigned int shaderProgram);
    void UpdateLightprobeSSBO();
   
    private:
    std::vector<Lightprobe> probes;
      bool sceneChanged = false;
      bool isRenderingCubemaps = false;
      float lastRenderTime     = 0.0f;
      unsigned int lightprobeSSBO = 0;
      std::vector<GPULightprobeData> currentGPUData;

      void RenderSingleProbeCubemap(Lightprobe& probe);
      bool CreateCubemapTexture(Lightprobe& probe);
      float4x4 CreateCubemapViewMatrix(const float3& position, const float3& forward, const float3& up);
      float4x4 CreateCubemapProjectionMatrix();
      void RenderSceneForCubemap(CameraComponent* camera);
      GPULightprobeData CreateGPUData(const Lightprobe& probe, const float3& size);
};
