#include "pch.h"

#include "Application.h"
#include "EditorUIModule.h"
#include "GameObject.h"
#include "Script.h"

#include "Math/float2.h"
#include "Math/float3.h"
#include "Math/float4.h"

void Script::Inspector()
{
    AppEngine->GetEditorUIModule()->DrawScriptInspector(fields);
}

void Script::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator)
{
    for (const auto& field : fields)
    {
        rapidjson::Value name(field.name, allocator);
        switch (field.type)
        {
        case InspectorField::FieldType::Float:
            targetState.AddMember(name, *(float*)field.data, allocator);
            break;
        case InspectorField::FieldType::Int:
            targetState.AddMember(name, *(int*)field.data, allocator);
            break;
        case InspectorField::FieldType::Bool:
            targetState.AddMember(name, *(bool*)field.data, allocator);
            break;
        case InspectorField::FieldType::Vec2:
        {
            float2* vec = (float2*)field.data;
            rapidjson::Value arr(rapidjson::kArrayType);
            arr.PushBack(vec->x, allocator);
            arr.PushBack(vec->y, allocator);
            targetState.AddMember(name, arr, allocator);
            break;
        }
        case InspectorField::FieldType::Vec3:
        case InspectorField::FieldType::Color: // Vec3 == Color
        {
            float3* vec = (float3*)field.data;
            rapidjson::Value arr(rapidjson::kArrayType);
            arr.PushBack(vec->x, allocator);
            arr.PushBack(vec->y, allocator);
            arr.PushBack(vec->z, allocator);
            targetState.AddMember(name, arr, allocator);
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
            targetState.AddMember(name, arr, allocator);
            break;
        }
        case InspectorField::FieldType::InputText:
        {
            std::string* str = static_cast<std::string*>(field.data);
            targetState.AddMember(name, rapidjson::Value(str->c_str(), allocator), allocator);
            break;
        }
        case InspectorField::FieldType::GameObject:
        {
            GameObject* go = *(GameObject**)field.data;
            UID uid        = go ? go->GetUID() : 0;
            targetState.AddMember(name, uid, allocator);
            break;
        }
        }
    }
}

void Script::Load(const rapidjson::Value& initialState)
{
    for (auto& field : fields)
    {
        if (!initialState.HasMember(field.name)) continue;

        const auto& value = initialState[field.name];

        switch (field.type)
        {
        case InspectorField::FieldType::Float:
            if (value.IsNumber()) *(float*)field.data = value.GetFloat();
            break;

        case InspectorField::FieldType::Int:
            if (value.IsInt()) *(int*)field.data = value.GetInt();
            break;

        case InspectorField::FieldType::Bool:
            if (value.IsBool()) *(bool*)field.data = value.GetBool();
            break;

        case InspectorField::FieldType::Vec2:
            if (value.IsArray() && value.Size() == 2)
            {
                float2* vec = (float2*)field.data;
                vec->x      = value[0].GetFloat();
                vec->y      = value[1].GetFloat();
            }
            break;

        case InspectorField::FieldType::Vec3:
        case InspectorField::FieldType::Color:
            if (value.IsArray() && value.Size() == 3)
            {
                float3* vec = (float3*)field.data;
                vec->x      = value[0].GetFloat();
                vec->y      = value[1].GetFloat();
                vec->z      = value[2].GetFloat();
            }
            break;

        case InspectorField::FieldType::Vec4:
            if (value.IsArray() && value.Size() == 4)
            {
                float4* vec = (float4*)field.data;
                vec->x      = value[0].GetFloat();
                vec->y      = value[1].GetFloat();
                vec->z      = value[2].GetFloat();
                vec->w      = value[3].GetFloat();
            }
            break;
        case InspectorField::FieldType::InputText:
        {
            if (value.IsString())
            {
                *(std::string*)field.data = value.GetString();
            }
            break;
        }
        case InspectorField::FieldType::GameObject:
            if (value.IsUint64())
            {
                UID uid = value.GetUint64();
                if (uid == 0) return;
                GameObject* go            = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(uid);
                *(GameObject**)field.data = go;
            }
            break;
        }
    }
}

void Script::CloneFields(const std::vector<InspectorField>& otherFields)
{
    for (size_t i = 0; i < fields.size(); ++i)
    {
        switch (fields[i].type)
        {
        case InspectorField::FieldType::Float:
            *(float*)fields[i].data = *(float*)otherFields[i].data;
            break;

        case InspectorField::FieldType::Int:
            *(int*)fields[i].data = *(int*)otherFields[i].data;
            break;

        case InspectorField::FieldType::Bool:
            *(bool*)fields[i].data = *(bool*)otherFields[i].data;
            break;

        case InspectorField::FieldType::Vec2:
            *(float2*)fields[i].data = *(float2*)otherFields[i].data;
            break;

        case InspectorField::FieldType::Vec3:
        case InspectorField::FieldType::Color:
            *(float3*)fields[i].data = *(float3*)otherFields[i].data;
            break;

        case InspectorField::FieldType::Vec4:
            *(float4*)fields[i].data = *(float4*)otherFields[i].data;
            break;
        case InspectorField::FieldType::InputText:
        {
            *(std::string*)fields[i].data = *(std::string*)otherFields[i].data;
            break;
        }
        case InspectorField::FieldType::GameObject:
            // It just works
            GameObject* uid = *(GameObject**)otherFields[i].data;
            if (uid == 0) return;
            GameObject* go                = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(uid->GetUID());
            *(GameObject**)fields[i].data = go;
            break;
        }
    }
}