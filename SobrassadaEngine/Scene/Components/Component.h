#pragma once

#include "Transform.h"
#include "ComponentUtils.h"
#include "Globals.h"
#include "debug_draw.hpp"
#include "Scene/AABBUpdatable.h"

#include <vector>
#include <Geometry/AABB.h>

class Component : public AABBUpdatable
{
public:

    Component(UID uid, UID uidParent, UID uidRoot, const char* name, const Transform& parentGlobalTransform);

    virtual ~Component();
    
    virtual void Update() = 0;
    virtual void Render();

    virtual bool AddChildComponent(UID componentUID);
    virtual bool RemoveChildComponent(UID componentUID);
    virtual bool DeleteChildComponent(UID componentUID);
    
    virtual void RenderEditorInspector();
    virtual void RenderEditorComponentTree(UID selectedComponentUID);

    virtual void OnTransformUpdate(const Transform &parentGlobalTransform);
    virtual AABB& TransformUpdated(const Transform &parentGlobalTransform);
    void PassAABBUpdateToParent() override;

    void HandleDragNDrop();

    UID  GetUID() const { return uid; }

    UID  GetUIDParent() const { return uidParent; }

    void SetUIDParent(UID newUIDParent) { uidParent = newUIDParent; }

    const Transform& GetGlobalTransform() const override { return globalTransform; }
    const Transform& GetLocalTransform() const { return localTransform; }

    const AABB& GetGlobalAABB() const { return globalComponentAABB; }

    void CalculateLocalAABB();

protected:
    
    const UID uid;
    const UID uidRoot;
    UID uidParent;
    std::vector<UID> children;

    const char* name;
    bool enabled;
    
    Transform localTransform;
    Transform globalTransform;

    AABB localComponentAABB;
    AABB globalComponentAABB;
    
};
