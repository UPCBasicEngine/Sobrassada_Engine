#pragma once

#include "Transform.h"
#include "ComponentUtils.h"
#include "debug_draw.hpp"
#include "Scene/AABBUpdatable.h"

#include <vector>
#include <Geometry/AABB.h>

class Component : public AABBUpdatable
{
public:

    Component(uint32_t uuid, uint32_t uuidParent, uint32_t uuidRoot, const char* name, const Transform& parentGlobalTransform);

    virtual ~Component();
    
    virtual void Update() = 0;
    virtual void Render();

    virtual bool AddChildComponent(const uint32_t componentUUID);
    virtual bool RemoveChildComponent(const uint32_t componentUUID);
    virtual bool DeleteChildComponent(const uint32_t componentUUID);
    
    virtual void RenderEditorInspector();
    virtual void RenderEditorComponentTree(const uint32_t selectedComponentUUID);

    virtual void OnTransformUpdate(const Transform &parentGlobalTransform);
    virtual AABB& TransformUpdated(const Transform &parentGlobalTransform);
    void PassAABBUpdateToParent() override;

    void HandleDragNDrop();

    uint32_t GetUUID() const { return uuid; }

    uint32_t GetUUIDParent() const { return uuidParent; }

    void SetUUIDParent(uint32_t newUUIDParent) { uuidParent = newUUIDParent; }

    const Transform& GetGlobalTransform() const override { return globalTransform; }
    const Transform& GetLocalTransform() const { return localTransform; }

    const AABB& GetGlobalAABB() const { return globalComponentAABB; }

    void CalculateLocalAABB();

protected:
    
    const uint32_t uuid;
    const uint32_t uuidRoot;
    uint32_t uuidParent;
    std::vector<uint32_t> children;

    const char* name;
    bool enabled;
    
    Transform localTransform;
    Transform globalTransform;

    AABB localComponentAABB;
    AABB globalComponentAABB;
    
};
