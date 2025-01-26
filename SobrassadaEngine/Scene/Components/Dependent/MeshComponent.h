#pragma once
#include "Scene/Components/Component.h"

class MeshComponent : public Component
{
public:
    MeshComponent(const uint32_t uuid, const uint32_t ownerUUID, const char* name);

    void RenderEditorInspector() override;
    void RenderEditorComponentTree(uint8_t layer) override;
    
    void Update() override;
};
