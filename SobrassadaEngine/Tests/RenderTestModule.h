#pragma once

#include "Components/ComponentMaterial.h"
#include "Module.h"
class EngineModel;
class LightsConfig;
class PointLight;

class RenderTestModule : public Module
{
  public:
    RenderTestModule();
    ~RenderTestModule();

    bool Init() override;
    update_status PreUpdate(float deltaTime) override;
    update_status Render(float deltaTime) override;
    bool ShutDown() override;

  private:
    void RenderEditorViewport();

    int program                = -1;
    unsigned int vbo           = -1;
    unsigned int baboonTexture = -1;
    EngineModel *currentLoadedModel;
    std::vector<ComponentMaterial *> materials;

    float3 lightDir = float3(-1.0f, -0.3f, 2.0f);
    LightsConfig *lightsConfig;
    PointLight *pointLight;
};
