#pragma once
#include "Scene/Components/Component.h"

enum ComponentMobilitySettings
{
    STATIC = 0,
    DYNAMIC = 1,
};

class RootComponent : public Component
{
public:
    RootComponent(UID uid, UID uidParent, const Transform& parentGlobalTransform);

    RootComponent(const rapidjson::Value &initialState);

    ~RootComponent() override;

    void Save(rapidjson::Value &targetState, rapidjson::Document::AllocatorType &allocator) const override;

    AABB & TransformUpdated(const Transform &parentGlobalTransform) override;

    virtual bool CreateComponent(ComponentType componentType);
    
    void RenderComponentEditor();
    void RenderEditorComponentTree(UID selectedComponentUID) override;
    void RenderEditorInspector() override;
    void RenderGuizmo();

    void Update() override;

    void SetSelectedComponent(UID componentUID);
    bool IsSelectedComponent(UID componentUID) const { return selectedUID == componentUID; }
    UID GetUID() { return selectedUID; }

    int GetMobilitySettings() const { return mobilitySettings; }

private:
    
    UID selectedUID;
    int mobilitySettings = DYNAMIC;
};
