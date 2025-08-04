#include "LightprobeManager.h"
#include "glew.h"
#include <algorithm>

LightprobeManager::LightprobeManager()
{
}

LightprobeManager::~LightprobeManager()
{
}

void LightprobeManager::AddProbe()
{
}

void LightprobeManager::RenderCubemaps()
{
}

std::vector<GPUProbeData> LightprobeManager::FindRelevantProbes(const float3& fragmentWorldPos, int maxProbes)
{
    std::vector<GPUProbeData> relevant;

    for (const Lightprobe& probe : sceneLightprobes)
    {
        float weight = probe.GetWeightAt(fragmentWorldPos);
        if (weight > 0.0f)
        {
            relevant.push_back(probe.PackToGPU(fragmentWorldPos));
        }
    }

    std::sort(
        relevant.begin(), relevant.end(),
        [](const GPUProbeData& a, const GPUProbeData& b) { return a.weight > b.weight; }
    );

    if (relevant.size() > static_cast<size_t>(maxProbes))
    {
        relevant.resize(maxProbes);
    }

    // Normalizar pesos
    float total = 0.0f;
    for (const auto& p : relevant)
        total += p.weight;
    if (total > 0.0f)
    {
        for (auto& p : relevant)
            p.weight /= total;
    }

    return relevant;
 }



void LightprobeManager::UploadToShader()
{
    GLuint ssbo = lightprobeSSBO;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER, sizeof(GPUProbeData) * outProbesGPU.size(), outProbesGPU.data(), GL_DYNAMIC_DRAW
    );
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo); // binding = 3 en el shader
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
