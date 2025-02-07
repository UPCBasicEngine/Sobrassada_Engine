#pragma once

#include "Transform.h"
#include "ComponentUtils.h"

#include <vector>

class Component
{
public:

    Component(const uint32_t uuid, const uint32_t uuidParent, const uint32_t uuidRoot, const char* name);

    virtual ~Component() = default;
    
    virtual void Enable();
    virtual void Update() = 0;
    virtual void Render();
    virtual void Disable();

    virtual bool AddComponent(const uint32_t componentUUID);
    virtual bool RemoveComponent(const uint32_t componentUUID);
    
    virtual void RenderEditorInspector();
    virtual void RenderEditorComponentTree(const uint32_t selectedComponentUUID);

    uint32_t GetUUID() const { return uuid; }

    uint32_t GetUUIDParent() const { return uuidParent; }

protected:
    
    const uint32_t uuid;
    const uint32_t uuidParent;
    const uint32_t uuidRoot;
    std::vector<uint32_t> children;

    const char* name;
    bool enabled;
    
    Transform localTransform;
    Transform globalTransform;
    
};
