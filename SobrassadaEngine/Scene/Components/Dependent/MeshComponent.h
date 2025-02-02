#pragma once
#include "Scene/Components/Component.h"

class MeshComponent : public Component
{
public:
    MeshComponent(const uint32_t uuid, const uint32_t uuidParent, const uint32_t uuidRoot, const char* name);

    void RenderEditorInspector() override;
    void RenderEditorComponentTree(const uint32_t selectedComponentUUID) override;
    
    void Update() override;
};
