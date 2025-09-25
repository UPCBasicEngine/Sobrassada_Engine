#pragma once
#include "EngineEditorBase.h"
#include "Math/float3.h"
class LightprobeManager;
class LightprobeEditor : public EngineEditorBase
{
  public:
    LightprobeEditor(const std::string& name, UID uid) : EngineEditorBase(name, uid) {};
    ~LightprobeEditor() {};
    bool RenderEditor() override;
    void SetLightprobeManager(LightprobeManager* manager) { lightprobeManager = manager; }

    private:
    LightprobeManager* lightprobeManager = nullptr;

    int selectedProbeIndex               = -1;
    float3 newProbePosition              = float3(0, 0, 0);
    float3 newProbeSize                  = float3(5, 5, 5);
    bool showCubemapPreview              = false;
    void RenderProbesList();
    void RenderProbeProperties();
    void RenderVisualization();
};
