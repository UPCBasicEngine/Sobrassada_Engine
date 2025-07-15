#include "ShaderScriptModule.h"

ShaderScriptModule::ShaderScriptModule()
{
}

ShaderScriptModule::~ShaderScriptModule()
{
}

bool ShaderScriptModule::Init()
{
    return true;
}

bool ShaderScriptModule::ShutDown()
{
    return true;
}

void ShaderScriptModule::AddShaderScript(
    ShaderScriptComponent* component, unsigned int scriptIndex, ShaderScriptType shaderType
)
{
}

void ShaderScriptModule::ShaderScriptTypeChange(
    ShaderScriptComponent* component, unsigned int scriptIndex, ShaderScriptType previous, ShaderScriptType newType
)
{
}

void ShaderScriptModule::ComponentDeleted(ShaderScriptComponent* component)
{
}
