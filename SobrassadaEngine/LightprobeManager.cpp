#include "LightprobeManager.h"
#include "Application.h"
#include "EngineTimer.h"
#include "Globals.h"
#include "glew.h"

LightprobeManager::~LightprobeManager()
{
    for (auto& probe : probes)
    {
        if (probe.cubemapTexture != 0)
        {
            glDeleteTextures(1, &probe.cubemapTexture);
        }
    }
}

void LightprobeManager::AddProbe(const float3& position, const float3& size)
{
    Lightprobe probe;
    probe.position = position;
    probe.size     = size;
    probe.needUpdate = true;
    probes.push_back(probe);
}

void LightprobeManager::RemoveProbe(int index)
{
    if (index >= 0 && index < probes.size())
    {
        if (probes[index].cubemapTexture != 0)
        {
            glDeleteTextures(1, &probes[index].cubemapTexture);
        }
        probes.erase(probes.begin() + index);
    }
}

void LightprobeManager::RenderCubemaps()
{
    if (isRenderingCubemaps) return; 

    float currentTime = App->GetEngineTimer()->GetTime();
    if (currentTime - lastRenderTime < 1000.0f) return; 

    isRenderingCubemaps = true;

    for (auto& probe : probes)
    {
        if (probe.needUpdate)
        {
            RenderSingleProbeCubemap(probe);
            probe.needUpdate = false;
        }
    }

    lastRenderTime      = currentTime;
    isRenderingCubemaps = false;
}

void LightprobeManager::RenderSingleProbeCubemap(Lightprobe& probe)
{
  
    if (!CreateCubemapTexture(probe))
    {
        GLOG("Failed to create cubemap for lightprobe");
        return;
    }

  
    GLOG(
        "Created cubemap texture %u for lightprobe at (%.1f, %.1f, %.1f)", probe.cubemapTexture, probe.position.x,
        probe.position.y, probe.position.z
    );
}


bool LightprobeManager::CreateCubemapTexture(Lightprobe& probe)
{
    if (probe.cubemapTexture != 0)
    {
        glDeleteTextures(1, &probe.cubemapTexture);
        probe.cubemapTexture = 0;
    }

    glGenTextures(1, &probe.cubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, probe.cubemapTexture);

  
    for (int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 256, 256, 0, GL_RGB, GL_FLOAT, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

  
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        GLOG("OpenGL error creating cubemap: %d", error);
        if (probe.cubemapTexture != 0)
        {
            glDeleteTextures(1, &probe.cubemapTexture);
            probe.cubemapTexture = 0;
        }
        return false;
    }

    return true;
}
