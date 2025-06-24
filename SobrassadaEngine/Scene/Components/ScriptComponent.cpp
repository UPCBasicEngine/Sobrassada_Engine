#include "ScriptComponent.h"

#include "Application.h"
#include "EditorUIModule.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "SceneModule.h"
#include "Script.h"
#include "ScriptModule.h"

#include "ImGui.h"
#include "Math/float3.h"

ScriptComponent::ScriptComponent(UID uid, GameObject* parent) : Component(uid, parent, "Script", COMPONENT_SCRIPT)
{
}

ScriptComponent::ScriptComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    if (initialState.HasMember("Scripts") && initialState["Scripts"].IsArray())
    {
        for (const auto& scriptData : initialState["Scripts"].GetArray())
        {
            if (scriptData.HasMember("Script Name"))
            {
                const char* name = scriptData["Script Name"].GetString();
                if (CreateScript(name))
                {
                    scriptInstances.back()->Load(scriptData);
                    if (scriptData.HasMember("Enabled")) scriptEnabled.back() = scriptData["Enabled"].GetBool();
                    if (scriptData.HasMember("WasEnabled"))
                        scriptWasEnabledLastFrame.back() = scriptData["WasEnabled"].GetBool();
                }
            }
        }
    }
}

void ScriptComponent::Load(const rapidjson::Value& initialState)
{
    if (initialState.HasMember("Scripts") && initialState["Scripts"].IsArray())
    {
        for (const auto& scriptData : initialState["Scripts"].GetArray())
        {
            if (scriptData.HasMember("Script Name"))
            {
                const char* name = scriptData["Script Name"].GetString();
                if (CreateScript(name))
                {
                    if (scriptData.HasMember("Enabled")) scriptEnabled.back() = scriptData["Enabled"].GetBool();
                    if (scriptData.HasMember("WasEnabled"))
                        scriptWasEnabledLastFrame.back() = scriptData["WasEnabled"].GetBool();
                    scriptInstances.back()->Load(scriptData);
                }
            }
        }
    }
}

ScriptComponent::~ScriptComponent()
{
    DeleteAllScripts();
}

void ScriptComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);
    rapidjson::Value scriptsArray(rapidjson::kArrayType);

    for (size_t i = 0; i < scriptInstances.size(); ++i)
    {
        rapidjson::Value scriptData(rapidjson::kObjectType);
        scriptData.AddMember("Script Name", rapidjson::Value(scriptNames[i].c_str(), allocator), allocator);
        scriptData.AddMember("Enabled", scriptEnabled[i], allocator);
        scriptData.AddMember("WasEnabled", scriptWasEnabledLastFrame[i], allocator);
        scriptInstances[i]->Save(scriptData, allocator);
        scriptsArray.PushBack(scriptData, allocator);
    }

    targetState.AddMember("Scripts", scriptsArray, allocator);
}

void ScriptComponent::Clone(const Component* other)
{
    if (other->GetType() == ComponentType::COMPONENT_SCRIPT)
    {
        const ScriptComponent* otherScript = static_cast<const ScriptComponent*>(other);
        enabled                            = otherScript->enabled;
        wasEnabled                         = otherScript->wasEnabled;
        DeleteAllScripts();

        for (size_t i = 0; i < otherScript->scriptNames.size(); ++i)
        {
            CreateScript(otherScript->scriptNames[i]);
            const auto& a = otherScript->scriptInstances[i]->GetFields();
            scriptInstances.back()->CloneFields(a);
        }
    }
    else
    {
        GLOG("It is not possible to clone a component of a different type!");
    }
}

void ScriptComponent::Update(float deltaTime)
{
    if (!IsEffectivelyEnabled()) return;

    if (App->GetSceneModule()->GetInPlayMode())
    {
        float gameTime = App->GetGameTimer()->GetDeltaTime() / 1000.0f; // seconds
        for (size_t i = 0; i < scriptInstances.size(); ++i)
        {
            if (scriptEnabled[i])
            {
                if (!scriptInitialized[i])
                {
                    scriptInstances[i]->Init();
                    scriptInitialized[i] = true;
                }

                scriptInstances[i]->Update(gameTime);
            }
        }
    }
}

void ScriptComponent::ResetInitializationFlags()
{
    std::fill(scriptInitialized.begin(), scriptInitialized.end(), false);
}

void ScriptComponent::Render(float deltaTime)
{
}

void ScriptComponent::RenderDebug(float deltaTime)
{
}

void ScriptComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();

    ImGui::SeparatorText("Script Component");
    if (ImGui::Button("Select script"))
    {
        ImGui::OpenPopup("Select Script");
    }

    if (ImGui::BeginPopup("Select Script"))
    {
        for (int i = 0; i < App->GetScriptModule()->GetScriptCount(); ++i)
        {
            const char* name = App->GetScriptModule()->GetScriptName(i);
            if (name && ImGui::Selectable(name))
            {
                CreateScript(name);
            }
        }
        ImGui::EndPopup();
    }

    for (int i = 0; i < scriptInstances.size(); ++i)
    {
        ImGui::Separator();
        ImGui::PushID(static_cast<int>(i));

        ImGui::Text(scriptNames[i].c_str());
        ImGui::SameLine();
        if (ImGui::Button("Delete"))
        {
            DeleteScript(i);
            ImGui::PopID();
            break;
        }

        if (scriptInstances[i])
        {
            ImGui::SameLine();
            if (parent->IsGloballyEnabled() && enabled)
            {
                bool isEnabled = scriptEnabled[i];

                if (ImGui::Checkbox("Enabled", &isEnabled))
                {
                    scriptWasEnabledLastFrame[i] = isEnabled;
                }
                scriptEnabled[i] = scriptWasEnabledLastFrame[i];
            }
            else
            {
                bool isEnabled = scriptEnabled[i];
                ImGui::Checkbox("Enabled", &isEnabled);
                scriptEnabled[i] = false;
            }

            scriptInstances[i]->Inspector();
        }

        ImGui::PopID();
    }
}

void ScriptComponent::InitScriptInstances()
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

void ScriptComponent::OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    for (auto& script : scriptInstances)
    {
        script->OnCollision(otherObject, collisionNormal, layer);
    }
}

bool ScriptComponent::CreateScript(const std::string& scriptType)
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

void ScriptComponent::DeleteScript(const int index)
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

void ScriptComponent::DeleteAllScripts()
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

void ScriptComponent::SetComponentEnabled(bool value)
{
    enabled = value;

    if (value)
    {

        for (size_t i = 0; i < scriptEnabled.size(); ++i)
        {
            scriptEnabled[i]     = scriptWasEnabledLastFrame[i];
            scriptInitialized[i] = false; // forçar un nou Init()
        }
    }
    else
    {
        for (size_t i = 0; i < scriptEnabled.size(); ++i)
        {

            if (scriptEnabled[i]) scriptWasEnabledLastFrame[i] = true;

            scriptEnabled[i] = false; // el Update s’atura
        }
    }
}
