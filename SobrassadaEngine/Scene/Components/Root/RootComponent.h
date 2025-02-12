#pragma once
#include "Scene/Components/Component.h"

class RootComponent : public Component
{
public:
    RootComponent(UID uid, UID uidParent, const char* name, const Transform& parentGlobalTransform);

    ~RootComponent() override;

    virtual bool CreateComponent(ComponentType componentType);
    
    bool AddChildComponent(UID componentUID) override;
    bool DeleteChildComponent( UID componentUID) override;
    void RenderComponentEditor();
    void RenderEditorComponentTree(UID selectedComponentUID) override;

    void Update() override;

    void SetSelectedComponent(UID componentUID);

private:
    
    UID selectedUID;
};
