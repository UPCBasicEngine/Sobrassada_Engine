#pragma once

#include "Transform.h"
#include "ComponentUtils.h"

#include <vector>

class Component
{
public:

    Component(const uint32_t uuid, const uint32_t ownerUUID, const char* name);

    virtual ~Component() = default;
    
    virtual void Enable();
    virtual void Update() = 0;
    virtual void Disable();

    virtual bool CreateComponent(const ComponentType componentType);
    virtual bool AddComponent(const uint32_t componentUUID);
    virtual bool RemoveComponent(const uint32_t componentUUID);
    
    virtual void RenderEditorInspector();
    virtual void RenderEditorComponentTree(uint8_t layer);

    uint32_t GetUUID() const;

protected:
    
    const uint32_t uuid;
    const uint32_t ownerUUID;
    std::vector<uint32_t> children;

    const char* name;
    bool enabled;
    
    Transform localTransform;
    Transform globalTransform;
    
};
