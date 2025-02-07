#pragma once
#include "Scene/Components/Component.h"

class RootComponent : public Component
{
public:
    RootComponent(const uint32_t uuid, const uint32_t uuidParent, const char* name);

    ~RootComponent() override;

    virtual bool CreateComponent(const ComponentType componentType);
    
    bool AddComponent(const uint32_t componentUUID) override;
    bool RemoveComponent(const uint32_t componentUUID) override;
    void RenderComponentEditor();
    void RenderEditorComponentTree(const uint32_t selectedComponentUUID) override;
    void RenderEditorInspector() override;

    void Update() override;

    void SetSelectedComponent(const uint32_t componentUUID);

private:
    
    uint32_t selectedUUID;
};
