#pragma once

#include "Module.h"
#include "Scene/Components/ComponentUtils.h"

#include <utility>
#include <vector>

class ShaderScriptComponent;

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

  private:
    std::vector<std::pair<ShaderScriptComponent*, unsigned int>> geometryPassComponents;
    std::vector<std::pair<ShaderScriptComponent*, unsigned int>> transparentComponents;
    std::vector<std::pair<ShaderScriptComponent*, unsigned int>> postLightingComponents;
};
