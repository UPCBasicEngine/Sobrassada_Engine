#include "ShaderScriptComponent.h"

#include "Application.h"
#include "CameraModule.h"
#include "Components/CameraComponent.h"
#include "EditorUIModule.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "SceneModule.h"
#include "Script.h"
#include "ScriptModule.h"

ShaderScriptComponent::ShaderScriptComponent(UID uid, GameObject* parent)
    : Component(uid, parent, "Shader script", COMPONENT_SHADER_SCRIPT)
{
    localComponentAABB = AABB(float3(-0.5, -0.5, -0.5), float3(0.5, 0.5, 0.5));
}

ShaderScriptComponent::ShaderScriptComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    localComponentAABB = AABB(float3(-0.5, -0.5, -0.5), float3(0.5, 0.5, 0.5));
}

ShaderScriptComponent::~ShaderScriptComponent()
{
}

void ShaderScriptComponent::Load(const rapidjson::Value& initialState)
{
}

void ShaderScriptComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);
}

void ShaderScriptComponent::Clone(const Component* other)
{
}

void ShaderScriptComponent::Update(float deltaTime)
{
}

void ShaderScriptComponent::Render(float deltaTime, CameraComponent* camera)
{
}

void ShaderScriptComponent::RenderDebug(float deltaTime)
{
}

void ShaderScriptComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();
}

void ShaderScriptComponent::InitScriptInstances()
{
    for (size_t i = 0; i < scriptInstances.size(); ++i)
    {
        if (scriptEnabled[i])
        {
            scriptInstances[i]->Init();
            scriptInitialized[i] = true;
        }
    }
}

bool ShaderScriptComponent::CreateScript(const std::string& scriptType)
{
    for (const std::string& name : scriptNames)
    {
        if (name == scriptType) return false;
    }

    Script* instance = App->GetScriptModule()->CreateScript(scriptType, parent);
    if (instance == nullptr) return false;

    scriptInstances.push_back(instance);
    scriptNames.push_back(scriptType);
    scriptTypes.push_back(App->GetScriptModule()->GetScriptIdx(scriptType));
    // scriptTypes.push_back(static_cast<ScriptType>(SearchIdxForString(scriptType)));
    scriptEnabled.push_back(true);
    scriptInitialized.push_back(false);
    scriptWasEnabledLastFrame.push_back(true);

    return true;
}

void ShaderScriptComponent::DeleteScript(const int index)
{
    if (index >= scriptInstances.size()) return;

    if (scriptInstances[index]) App->GetScriptModule()->DestroyScript(scriptInstances[index]);

    scriptInstances.erase(scriptInstances.begin() + index);
    scriptNames.erase(scriptNames.begin() + index);
    scriptTypes.erase(scriptTypes.begin() + index);

    scriptEnabled.erase(scriptEnabled.begin() + index);
    scriptInitialized.erase(scriptInitialized.begin() + index);
    scriptWasEnabledLastFrame.erase(scriptWasEnabledLastFrame.begin() + index);
}

void ShaderScriptComponent::DeleteAllScripts()
{
    for (auto& script : scriptInstances)
    {
        if (script) App->GetScriptModule()->DestroyScript(script);
    }

    scriptInstances.clear();
    scriptNames.clear();
    scriptTypes.clear();

    scriptEnabled.clear();
    scriptInitialized.clear();
    scriptWasEnabledLastFrame.clear();
}

void ShaderScriptComponent::SetComponentEnabled(bool value)
{
    enabled = value;

    if (value)
    {

        for (size_t i = 0; i < scriptEnabled.size(); ++i)
        {
            scriptEnabled[i]     = scriptWasEnabledLastFrame[i];
            scriptInitialized[i] = false;
        }
    }
    else
    {
        for (size_t i = 0; i < scriptEnabled.size(); ++i)
        {

            if (scriptEnabled[i]) scriptWasEnabledLastFrame[i] = true;

            scriptEnabled[i] = false;
        }
    }
}