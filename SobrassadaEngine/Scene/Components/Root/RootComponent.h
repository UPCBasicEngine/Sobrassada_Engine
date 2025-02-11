#pragma once
#include "Scene/Components/Component.h"

class RootComponent : public Component
{
public:
    RootComponent(uint32_t uuid, uint32_t uuidParent, const char* name, const Transform& parentGlobalTransform);

    ~RootComponent() override;

    virtual bool CreateComponent(const ComponentType componentType);
    
    bool AddChildComponent(const uint32_t componentUUID) override;
    bool DeleteChildComponent(const uint32_t componentUUID) override;
    void RenderComponentEditor();
    void RenderEditorComponentTree(const uint32_t selectedComponentUUID) override;

    void Update() override;

    void SetSelectedComponent(const uint32_t componentUUID);

private:
    
    uint32_t selectedUUID;
};
