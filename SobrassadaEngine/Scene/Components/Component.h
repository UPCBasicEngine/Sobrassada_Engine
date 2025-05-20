#pragma once

#include "ComponentUtils.h"
#include "Globals.h"

#include "Geometry/AABB.h"
#include "rapidjson/document.h"

class GameObject;

class SOBRASADA_API_ENGINE Component
{
  public:
    Component(UID uid, GameObject* parent, const char* initName, ComponentType type);
    Component(const rapidjson::Value& initialState, GameObject* parent);
    virtual ~Component() = default;

    virtual void Init() {}

    virtual void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const;
    virtual void Clone(const Component* other) = 0;

    virtual void Update(float deltaTime)       = 0;
    virtual void Render(float deltaTime)       = 0;
    virtual void RenderDebug(float deltaTime)  = 0;
    virtual void RenderEditorInspector();
    virtual void ParentUpdated() { return; };

    UID GetUID() const { return uid; }
    UID GetParentUID() const;
    GameObject* GetParent() const { return parent; }

    const AABB& GetLocalAABB() const { return localComponentAABB; }

    ComponentType GetType() const { return type; }
    const char* GetName() const { return name; }
    const bool GetEnabled() const { return enabled; }

    const float4x4& GetGlobalTransform() const;
    bool IsEffectivelyEnabled() const;

    void SetEnabled(bool newEnabled) { enabled = newEnabled; }

  protected:
    const UID uid;
    GameObject* parent = nullptr;

    char name[64];
    bool enabled = true;
    bool wasEnabled = true;

    AABB localComponentAABB;

    const ComponentType type = COMPONENT_NONE;
};
