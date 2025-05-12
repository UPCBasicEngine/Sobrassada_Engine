#pragma once
#include "Math/float3.h"
#include "rapidjson/document.h"

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
        GameObject
    };

    const char* name;
    FieldType type;
    void* data;
    float minValue;
    float maxValue;
};

// Here we only need to add the functions that are going to be used in the ScriptModule
class Script
{
  public:
    virtual ~Script() {}

    virtual bool Init()                                                                             = 0;
    virtual void Update(float deltaTime)                                                            = 0;
    virtual void Inspector()                                                                        = 0;
    virtual void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) = 0;
    virtual void Load(const rapidjson::Value& initialState)                                         = 0;
    virtual void CloneFields(const std::vector<InspectorField>& fields)                             = 0;
    virtual void OnCollision(GameObject* otherObject, const float3& collisionNormal)                = 0;
    virtual void OnDestroy() {};

    virtual const std::vector<InspectorField>& GetFields() = 0;
    virtual void SetFields(const std::vector<InspectorField>& newFields) {}
};