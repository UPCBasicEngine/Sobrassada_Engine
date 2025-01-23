#pragma once
#include "Transform.h"

#include <vector>

class Component
{
public:

    Component(char* ownerUUID, char* name);

    virtual ~Component() = default;
    
    virtual void Enable();
    virtual void Update();
    virtual void Disable();

    bool AddComponent(const char* componentUUID);
    bool RemoveComponent(const char* componentUUID);

    virtual void RenderEditor();
    
private:

    char* ownerUUID;
    std::vector<const char*> children;

    char* name;
    bool enabled;
    
    Transform localTransform;
    Transform globalTransform;
    
};
