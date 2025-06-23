#pragma once

#include "ComponentUtils.h"
#include "Math/float3.h"
#include "rapidjson/document.h"
#include <functional>
#include <vector>

class Script;
class GameObject;
class Application;

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

    InspectorField(const char* name, FieldType type, void* data, float minValue, float maxValue)
        : name(name), type(type), data(data), minValue(minValue), maxValue(maxValue)
    {
    }
    InspectorField(const char* name, FieldType type, void* data)
        : name(name), type(type), data(data), minValue(0.0f), maxValue(1.0f)
    {
    }
    InspectorField(FieldType type, void* data) : name("No name"), type(type), data(data), minValue(0.0f), maxValue(1.0f)
    {
    }
    InspectorField(const char* name, std::function<void(Script*)> callback)
        : name(name), type(FieldType::Button), data(nullptr), minValue(0.0f), maxValue(1.0f), callback(callback)
    {
    }
};

class Script
{
  public:
    Script(GameObject* gameObject) : parent(gameObject) {}
    virtual ~Script() noexcept { parent = nullptr; }

    virtual bool Init()                  = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Inspector();
    virtual void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator);
    virtual void Load(const rapidjson::Value& initialState);
    virtual void CloneFields(const std::vector<InspectorField>& fields);
    virtual void OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer) {};
    virtual void OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer) {};
    virtual void OnCollisionExit(GameObject* otherObject, ColliderLayer layer) {};
    virtual void OnDestroy() {};

    virtual const std::vector<InspectorField>& GetFields() const { return fields; }
    virtual void SetFields(const std::vector<InspectorField>& newFields) { fields = newFields; }

  protected:
    GameObject* parent;
    std::vector<InspectorField> fields;
};

extern Application* AppEngine;