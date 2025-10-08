#pragma once

#include "Module.h"
#include "Scene/Components/ComponentUtils.h"

#include <utility>
#include <vector>

class ShaderScriptComponent;
class CameraComponent;

class ShaderScriptModule : public Module
{
  public:
    ShaderScriptModule();
    ~ShaderScriptModule() override;

    bool Init() override;
    bool ShutDown() override;

    void AddShaderScript(ShaderScriptComponent* component, unsigned int scriptIndex, ShaderScriptType shaderType);
    void ShaderScriptTypeChange(
        ShaderScriptComponent* component, unsigned int scriptIndex, ShaderScriptType previous, ShaderScriptType newType
    );
    void ComponentDeleted(ShaderScriptComponent* component);
    void ComponentDeletedScript(ShaderScriptComponent* component);

    void RenderGeometryPassShaders(float deltaTime, CameraComponent* camera);
    void RenderTransparentPassShaders(float deltaTime, CameraComponent* camera);
    void RenderPostLightingPassShaders(float deltaTime, CameraComponent* camera);
    void RenderPostEffectsPassShaders(float deltaTime, CameraComponent* camera);
    void RenderPreUiPassShaders(float deltaTime);
    void RenderUiPassShaders(float deltaTime);

  private:
    std::vector<std::pair<ShaderScriptComponent*, unsigned int>> geometryPassComponents;
    std::vector<std::pair<ShaderScriptComponent*, unsigned int>> transparentComponents;
    std::vector<std::pair<ShaderScriptComponent*, unsigned int>> postLightingComponents;
    std::vector<std::pair<ShaderScriptComponent*, unsigned int>> postEffectsComponents;
    std::vector<std::pair<ShaderScriptComponent*, unsigned int>> preUiComponents;
    std::vector<std::pair<ShaderScriptComponent*, unsigned int>> uiComponents;
};
