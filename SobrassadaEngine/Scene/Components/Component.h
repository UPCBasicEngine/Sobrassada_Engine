#pragma once

#include "Transform.h"
#include "ComponentUtils.h"

#include <vector>

class Component
{
public:

    Component(uint32_t uuid, uint32_t uuidParent, uint32_t uuidRoot, const char* name, const Transform& parentGlobalTransform);

    virtual ~Component();
    
    virtual void Enable();
    virtual void Update() = 0;
    virtual void Render();
    virtual void Disable();

    virtual bool AddChildComponent(const uint32_t componentUUID);
    virtual bool RemoveChildComponent(const uint32_t componentUUID);
    virtual bool DeleteChildComponent(const uint32_t componentUUID);
    
    virtual void RenderEditorInspector();
    virtual void RenderEditorComponentTree(const uint32_t selectedComponentUUID);

    virtual void ParentGlobalTransformUpdated(const Transform& parentGlobalTransform);

    void HandleDragNDrop();

    uint32_t GetUUID() const { return uuid; }

    uint32_t GetUUIDParent() const { return uuidParent; }

    void SetUUIDParent(uint32_t newUUIDParent) { uuidParent = newUUIDParent; }

    const Transform& GetGlobalTransform() const { return globalTransform; }
    const Transform& GetLocalTransform() const { return localTransform; }

protected:

    virtual void TransformUpdated();
    
    const uint32_t uuid;
    const uint32_t uuidRoot;
    uint32_t uuidParent;
    std::vector<uint32_t> children;

    const char* name;
    bool enabled;
    
    Transform localTransform;
    Transform globalTransform;
    
};
