#pragma once

#include "ComponentUtils.h"
#include "Math/float2.h"
#include "Math/float3.h"
#include "Math/float4.h"
#include "rapidjson/document.h"
#include <functional>
#include <variant>

class GameObject;

struct InspectorField
{
    enum class FieldType
    {
        Text,
        Float,
        Int,
        Bool,
        Vec2,
        Vec3,
        Vec4,
        Color,
        InputText,
        GameObject,
        Button
    };

    std::string name;
    FieldType type;
    void* data;
    float minValue;
    float maxValue;
    std::function<void(Script*)> callback;
};

// Here we only need to add the functions that are going to be used in the ScriptModule
class Script
{
  public:
    virtual ~Script() {}

    virtual bool Init()                                                                                  = 0;
    virtual void Update(float deltaTime)                                                                 = 0;
    virtual void Inspector()                                                                             = 0;
    virtual void Load(const rapidjson::Value& initialState)                                              = 0;
    virtual void CloneFields(const std::vector<InspectorField>& fields)                                  = 0;
    virtual void OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer) = 0;
    virtual void OnDestroy() {};

    virtual const std::vector<InspectorField>& GetFields() = 0;
    virtual void SetFields(const std::vector<InspectorField>& newFields) {};
};