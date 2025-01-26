#pragma once
#include "Scene/Components/Component.h"

#include <cstdint>

class ModelComponent : public Component
{
public:
    ModelComponent(const uint32_t uuid, const uint32_t ownerUUID, const char* name);
    
    void RenderEditorInspector() override;
    void RenderEditorComponentTree(uint8_t layer) override;
    void Update() override;

    void LoadModel(uint32_t modelUUID); // TODO Call library module to load data from serialized model

private:

    // TODO Add model code, add mesh components when loading

    uint32_t currentModelUUID = 0; // TODO Define value for undefined UUID 
    
};
