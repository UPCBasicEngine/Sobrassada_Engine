#pragma once
#include "Scene/Components/Component.h"

class RootComponent : public Component
{
public:
    RootComponent(UID uid, UID uidParent, const Transform& parentGlobalTransform);

    RootComponent(const rapidjson::Value &initialState);

    ~RootComponent() override;

    void Save(rapidjson::Value &targetState, rapidjson::Document::AllocatorType &allocator) const override;

    virtual bool CreateComponent(ComponentType componentType);
    
    void RenderComponentEditor();
    void RenderEditorComponentTree(UID selectedComponentUID) override;

    void Update() override;

    void SetSelectedComponent(UID componentUID);

private:
    
    UID selectedUID;
};
