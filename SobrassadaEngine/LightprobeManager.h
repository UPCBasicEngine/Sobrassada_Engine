#pragma once
#include "Lightprobe.h"
#include <vector>
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

    private:
    std::vector<Lightprobe> probes;
      bool sceneChanged = false;
    bool isRenderingCubemaps = false;
      float lastRenderTime     = 0.0f;

      void RenderSingleProbeCubemap(Lightprobe& probe);
      bool CreateCubemapTexture(Lightprobe& probe);
};
