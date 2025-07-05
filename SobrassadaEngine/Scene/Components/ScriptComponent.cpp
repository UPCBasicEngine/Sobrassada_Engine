#include "ScriptComponent.h"

#include "Application.h"
#include "EditorUIModule.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "SceneModule.h"
#include "Script.h"
#include "ScriptModule.h"

#include "ImGui.h"
#include "Math/float2.h"
#include "Math/float3.h"
#include "PhysicsModule.h"
#include <debug_draw.hpp>

ScriptComponent::ScriptComponent(UID uid, GameObject* parent) : Component(uid, parent, "Script", COMPONENT_SCRIPT)
{
    localComponentAABB = AABB(float3(-0.5, -0.5, -0.5), float3(0.5, 0.5, 0.5));
}

ScriptComponent::ScriptComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    localComponentAABB = AABB(float3(-0.5, -0.5, -0.5), float3(0.5, 0.5, 0.5));

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
        GLOG("Script Name: %s", scriptNames[i].c_str());

        const auto& fields = scriptInstances[i]->GetFields();
        for (const auto field : fields)
        {
            rapidjson::Value fieldData(rapidjson::kObjectType);

            rapidjson::Value name(field.name.c_str(), allocator);
            switch (field.type)
            {
            case InspectorField::FieldType::Float:
                scriptData.AddMember(name, *(float*)field.data, allocator);
                break;
            case InspectorField::FieldType::Int:
                scriptData.AddMember(name, *(int*)field.data, allocator);
                break;
            case InspectorField::FieldType::Bool:
                scriptData.AddMember(name, *(bool*)field.data, allocator);
                break;
            case InspectorField::FieldType::Vec2:
            {
                float2* vec = (float2*)field.data;
                rapidjson::Value arr(rapidjson::kArrayType);
                arr.PushBack(vec->x, allocator);
                arr.PushBack(vec->y, allocator);
                scriptData.AddMember(name, arr, allocator);
                break;
            }
            case InspectorField::FieldType::Vec3:
            {
                float3* vec = (float3*)field.data;
                rapidjson::Value arr(rapidjson::kArrayType);
                arr.PushBack(vec->x, allocator);
                arr.PushBack(vec->y, allocator);
                arr.PushBack(vec->z, allocator);
                scriptData.AddMember(name, arr, allocator);
                break;
            }
            case InspectorField::FieldType::Vec4:
            {
                float4* vec = (float4*)field.data;
                rapidjson::Value arr(rapidjson::kArrayType);
                arr.PushBack(vec->x, allocator);
                arr.PushBack(vec->y, allocator);
                arr.PushBack(vec->z, allocator);
                arr.PushBack(vec->w, allocator);
                scriptData.AddMember(name, arr, allocator);
                break;
            }
            case InspectorField::FieldType::InputText:
            {
                std::string* str = static_cast<std::string*>(field.data);
                scriptData.AddMember(name, rapidjson::Value(str->c_str(), allocator), allocator);
                break;
            }
            case InspectorField::FieldType::GameObject:
            {
                GameObject* go = *(GameObject**)field.data;
                UID uid        = go ? go->GetUID() : 0;
                scriptData.AddMember(name, uid, allocator);
            }
            }
        }

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
            const auto& fields = otherScript->scriptInstances[i]->GetFields();
            scriptInstances.back()->CloneFields(fields);
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

void ScriptComponent::OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    for (auto& script : scriptInstances)
    {
        script->OnCollisionEnter(otherObject, collisionNormal, layer);
    }
}

void ScriptComponent::OnCollisionExit(GameObject* otherObject, ColliderLayer layer)
{
    for (auto& script : scriptInstances)
    {
        script->OnCollisionExit(otherObject, layer);
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
