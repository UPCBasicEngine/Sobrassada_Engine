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
#include "ShaderScriptModule.h"

ShaderScriptComponent::ShaderScriptComponent(UID uid, GameObject* parent)
    : Component(uid, parent, "Shader script", COMPONENT_SHADER_SCRIPT)
{
    localComponentAABB = AABB(float3(-0.5, -0.5, -0.5), float3(0.5, 0.5, 0.5));
}

ShaderScriptComponent::ShaderScriptComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    localComponentAABB = AABB(float3(-0.5, -0.5, -0.5), float3(0.5, 0.5, 0.5));

    if (initialState.HasMember("Scripts") && initialState["Scripts"].IsArray())
    {
        for (const auto& scriptData : initialState["Scripts"].GetArray())
        {
            if (scriptData.HasMember("Script Name"))
            {
                const char* name                  = scriptData["Script Name"].GetString();

                ShaderScriptType scriptRenderType = ShaderScriptType::NONE;
                if (scriptData.HasMember("RenderType"))
                    scriptRenderType = ShaderScriptType(scriptData["RenderType"].GetInt());

                if (CreateScript(name, scriptRenderType))
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

ShaderScriptComponent::~ShaderScriptComponent()
{
    DeleteAllScripts();
}

void ShaderScriptComponent::Load(const rapidjson::Value& initialState)
{
    if (initialState.HasMember("Scripts") && initialState["Scripts"].IsArray())
    {
        for (const auto& scriptData : initialState["Scripts"].GetArray())
        {
            if (scriptData.HasMember("Script Name"))
            {
                const char* name                  = scriptData["Script Name"].GetString();

                ShaderScriptType scriptRenderType = ShaderScriptType::NONE;
                if (scriptData.HasMember("RenderType"))
                    scriptRenderType = ShaderScriptType(scriptData["RenderType"].GetInt());

                if (CreateScript(name, scriptRenderType))
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

void ShaderScriptComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);
    rapidjson::Value scriptsArray(rapidjson::kArrayType);

    for (size_t i = 0; i < scriptInstances.size(); ++i)
    {
        rapidjson::Value scriptData(rapidjson::kObjectType);
        scriptData.AddMember("Script Name", rapidjson::Value(scriptNames[i].c_str(), allocator), allocator);
        scriptData.AddMember("Enabled", scriptEnabled[i], allocator);
        scriptData.AddMember("WasEnabled", scriptWasEnabledLastFrame[i], allocator);
        scriptData.AddMember("RenderType", (int)shaderScriptRenderType[i], allocator);
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
                break;
            }
            case InspectorField::FieldType::Resource:
            {
                scriptData.AddMember(name, *(UID*)field.data, allocator);
                break;
            }
            }
        }

        scriptsArray.PushBack(scriptData, allocator);
    }

    targetState.AddMember("Scripts", scriptsArray, allocator);
}

void ShaderScriptComponent::Clone(const Component* other)
{
    if (other->GetType() == ComponentType::COMPONENT_SHADER_SCRIPT)
    {
        const ShaderScriptComponent* otherScript = static_cast<const ShaderScriptComponent*>(other);
        enabled                                  = otherScript->enabled;
        wasEnabled                               = otherScript->wasEnabled;
        DeleteAllScripts();

        for (size_t i = 0; i < otherScript->scriptNames.size(); ++i)
        {
            CreateScript(otherScript->scriptNames[i], otherScript->shaderScriptRenderType[i]);
            const auto& fields = otherScript->scriptInstances[i]->GetFields();
            scriptInstances.back()->CloneFields(fields);
        }
    }
    else
    {
        GLOG("It is not possible to clone a component of a different type!");
    }
}

void ShaderScriptComponent::Update(float deltaTime)
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

void ShaderScriptComponent::Render(float deltaTime, CameraComponent* camera)
{
    if (!IsEffectivelyEnabled()) return;

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

            scriptInstances[i]->Render(gameTime, camera);
        }
    }
}

void ShaderScriptComponent::RenderScript(float deltaTime, CameraComponent* camera, int scriptIndex)
{
    if (!enabled || scriptIndex >= scriptInstances.size()) return;

    float gameTime = App->GetGameTimer()->GetDeltaTime() / 1000.0f; // seconds

    if (scriptEnabled[scriptIndex])
    {
        if (!scriptInitialized[scriptIndex])
        {
            scriptInstances[scriptIndex]->Init();
            scriptInitialized[scriptIndex] = true;
        }

        scriptInstances[scriptIndex]->Render(gameTime, camera);
    }
}

void ShaderScriptComponent::RenderDebug(float deltaTime)
{
}

void ShaderScriptComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();

    ImGui::SeparatorText("Shader Script Component");
    if (ImGui::Button("Select shader script"))
    {
        ImGui::OpenPopup("Select shader Script");
    }

    if (ImGui::BeginPopup("Select shader Script"))
    {
        for (int i = 0; i < App->GetScriptModule()->GetShaderScriptCount(); ++i)
        {
            const char* name = App->GetScriptModule()->GetShaderScriptName(i);
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

            char currentSrciptTag[50];
            snprintf(currentSrciptTag, 50, "Render type##%d", i);
            if (ImGui::BeginCombo(currentSrciptTag, ShaderScriptTypeStrings[(int)shaderScriptRenderType[i]]))
            {
                ShaderScriptType previous = shaderScriptRenderType[i];
                for (int stringIndex = 0; stringIndex < ShaderScriptTypeStringsSize; ++stringIndex)
                {
                    if (ImGui::Selectable(ShaderScriptTypeStrings[stringIndex]))
                    {
                        shaderScriptRenderType[i] = ShaderScriptType(stringIndex);
                        // CALL FUNCTION TO MODULE TO MODIFY VECTORS
                        App->GetShaderScriptModule()->ShaderScriptTypeChange(
                            this, i, previous, shaderScriptRenderType[i]
                        );
                    }
                }
                ImGui::EndCombo();
            }

            scriptInstances[i]->Inspector();
        }

        ImGui::PopID();
    }
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

bool ShaderScriptComponent::CreateScript(const std::string& scriptType, ShaderScriptType renderType)
{
    for (const std::string& name : scriptNames)
    {
        if (name == scriptType) return false;
    }

    Script* instance = App->GetScriptModule()->CreateScript(scriptType, parent);
    if (instance == nullptr) return false;

    scriptInstances.push_back(instance);
    scriptNames.push_back(scriptType);
    scriptTypes.push_back(App->GetScriptModule()->GetShaderScriptIdx(scriptType));
    scriptEnabled.push_back(true);
    scriptInitialized.push_back(false);
    scriptWasEnabledLastFrame.push_back(true);

    if (renderType != ShaderScriptType::NONE) shaderScriptRenderType.push_back(renderType);
    else shaderScriptRenderType.push_back(ShaderScriptType::GEOMERTY_PASS);
    
    unsigned int scriptIndex = (unsigned int)scriptInstances.size() - 1;
    App->GetShaderScriptModule()->AddShaderScript(this, scriptIndex, shaderScriptRenderType[scriptIndex]);

    return true;
}

void ShaderScriptComponent::DeleteScript(const int index)
{
    if (index >= scriptInstances.size()) return;

    if (scriptInstances[index]) App->GetScriptModule()->DestroyScript(scriptInstances[index]);
    
    App->GetShaderScriptModule()->ComponentDeleted(this);

    scriptInstances.erase(scriptInstances.begin() + index);
    scriptNames.erase(scriptNames.begin() + index);
    scriptTypes.erase(scriptTypes.begin() + index);

    scriptEnabled.erase(scriptEnabled.begin() + index);
    scriptInitialized.erase(scriptInitialized.begin() + index);
    scriptWasEnabledLastFrame.erase(scriptWasEnabledLastFrame.begin() + index);
    shaderScriptRenderType.erase(shaderScriptRenderType.begin() + index);

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
    shaderScriptRenderType.clear();

    App->GetShaderScriptModule()->ComponentDeleted(this);
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