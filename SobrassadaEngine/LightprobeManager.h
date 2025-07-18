#pragma once
#include "Geometry/OBB.h"
#include "math/float4x4.h"
#include "math/float4.h"
#include "Globals.h"
#include <vector>

struct GPUProbeData
{
    float3 position;
    float3 axisX;
    float3 axisY;
    float3 axisZ;
    float3 halfSize;
    float3 influenceMin;
    float3 influenceMax;
    int cubemapIndex;
    float weight;
};
struct Lightprobe
{
    float3 position;
    AABB proxyVolume;
    OBB influenceVolume;
    unsigned int sampleCubemap;
    float minBound;
    float maxBound;
    bool isDynamic;

    float GetWeightAt(const float3& fragmentWorldPos) const {
        float3 toCenter = fragmentWorldPos - influenceVolume.pos;
        float localX    = Dot(toCenter, influenceVolume.axis[0]);
        float localY    = Dot(toCenter, influenceVolume.axis[1]);
        float localZ    = Dot(toCenter, influenceVolume.axis[2]);

        if (abs(localX) > influenceVolume.HalfSize().x) return 0.0f;
        if (abs(localY) > influenceVolume.HalfSize().y) return 0.0f;
        if (abs(localZ) > influenceVolume.HalfSize().z) return 0.0f;

        //calculate weight
        float dx = 1.0f - abs(localX) / influenceVolume.HalfSize().x;
        float dy = 1.0f - abs(localY) / influenceVolume.HalfSize().y;
        float dz = 1.0f - abs(localZ) / influenceVolume.HalfSize().z;

        float minEdge = std::min({dx, dy, dz});
        float weight  = minEdge * minEdge;

        return weight;
    }

    GPUProbeData PackToGPU(const float3& fragmentWorldPos) const { 
        GPUProbeData out;
        out.position = influenceVolume.pos;
        out.axisX    = influenceVolume.axis[0];
        out.axisY    = influenceVolume.axis[1];
        out.axisZ    = influenceVolume.axis[2];
        out.halfSize = influenceVolume.HalfSize();
        out.influenceMin = proxyVolume.minPoint;
        out.influenceMax = proxyVolume.maxPoint;
        out.cubemapIndex = sampleCubemap;
        out.weight       = GetWeightAt(fragmentWorldPos);
        return out;
    }
};


class LightprobeManager
{
  private:
    unsigned int lightprobeSSBO;
    std::vector<GPUProbeData> outProbesGPU;
  public:
    LightprobeManager();
    ~LightprobeManager();
    std::vector<Lightprobe> sceneLightprobes;
    void AddProbe();
    void RenderCubemaps();
    std::vector<GPUProbeData> FindRelevantProbes(const float3& fragmentWorldPos, int maxProbes);
    void UploadToShader();
};
