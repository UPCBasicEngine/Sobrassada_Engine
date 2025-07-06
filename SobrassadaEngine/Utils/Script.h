#pragma once

#include "ComponentUtils.h"
#include "Math/float3.h"
#include "rapidjson/document.h"
#include <functional>

class GameObject;
class CameraComponent;

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

    const char* name;
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

    virtual bool Init()                                                                                       = 0;
    virtual void Update(float deltaTime)                                                                      = 0;
    virtual void Inspector()                                                                                  = 0;
    virtual void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator)           = 0;
    virtual void Load(const rapidjson::Value& initialState)                                                   = 0;
    virtual void CloneFields(const std::vector<InspectorField>& fields)                                       = 0;
    virtual void OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)      = 0;
    virtual void OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer) = 0;
    virtual void OnCollisionExit(GameObject* otherObject, ColliderLayer layer)                                = 0;
    virtual void OnPlayerExitLocation()                                                                       = 0;
    virtual void OnPlayerEnterLocation()                                                                      = 0;
    virtual void OnDestroy() {};

    virtual void Render(float deltaTime, CameraComponent* camera) = 0;

    virtual const std::vector<InspectorField>& GetFields() = 0;
    virtual void SetFields(const std::vector<InspectorField>& newFields) {}
};