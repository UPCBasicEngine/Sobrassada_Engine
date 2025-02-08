#pragma once
#include "Scene/Components/Component.h"

class MeshComponent : public Component
{
public:
    MeshComponent(uint32_t uuid, uint32_t uuidParent, uint32_t uuidRoot, const char* name, const Transform& parentGlobalTransform);

    void RenderEditorInspector() override;
    void RenderEditorComponentTree(const uint32_t selectedComponentUUID) override;
    
    void Update() override;
};
