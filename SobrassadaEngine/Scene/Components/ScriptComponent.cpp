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
                if (CreateScript(name)) scriptInstances.back()->Load(scriptData);
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
                if (CreateScript(name)) scriptInstances.back()->Load(scriptData);
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
        for (auto& script : scriptInstances)
        {
            script->Update(gameTime);
        }
    }
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
    if (enabled)
    {
        ImGui::SeparatorText("Script Component");
        if (ImGui::Button("Select script"))
        {
            ImGui::OpenPopup("Select Script");
        }
        if (ImGui::BeginPopup("Select Script"))
        {
            for (int i = 0; i < SCRIPT_TYPE_COUNT; ++i)
            {
                if (ImGui::Selectable(scripts[i]))
                {
                    CreateScript(scripts[i]);
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
                scriptInstances[i]->Inspector();
            }

            ImGui::PopID();
        }
    }
}

void ScriptComponent::InitScriptInstances()
{
    for (auto& script : scriptInstances)
    {
        script->Init();
    }
}

void ScriptComponent::OnCollision(GameObject* otherObject, const float3& collisionNormal)
{
    for (auto& script : scriptInstances)
    {
        script->OnCollision(otherObject, collisionNormal);
    }
}

bool ScriptComponent::CreateScript(const std::string& scriptType)
{
    Script* instance = App->GetScriptModule()->CreateScript(scriptType, parent);
    if (instance == nullptr) return false;

    scriptInstances.push_back(instance);
    scriptNames.push_back(scriptType);
    scriptTypes.push_back(static_cast<ScriptType>(SearchIdxForString(scriptType)));

    return true;
}

void ScriptComponent::DeleteScript(const int index)
{
    if (index >= scriptInstances.size()) return;

    if (scriptInstances[index]) App->GetScriptModule()->DestroyScript(scriptInstances[index]);

    scriptInstances.erase(scriptInstances.begin() + index);
    scriptNames.erase(scriptNames.begin() + index);
    scriptTypes.erase(scriptTypes.begin() + index);
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
}

int ScriptComponent::SearchIdxForString(const std::string& scriptString) const
{
    int idx = 0;
    for (int i = 0; i < SCRIPT_TYPE_COUNT; ++i)
    {
        if (scriptString == scripts[i])
        {
            idx = i;
            break;
        }
    }
    return idx;
}
