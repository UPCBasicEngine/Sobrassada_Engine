#pragma once
#include "Globals.h"
#include "Scene/Components/Component.h"

#include <cstdint>

class ModelComponent : public Component
{
public:
    ModelComponent(const uint32_t uuid, const uint32_t uuidParent, const uint32_t uuidRoot, const char* name);
    
    void RenderEditorInspector() override;
    void RenderEditorComponentTree(const uint32_t selectedComponentUUID) override;
    void Update() override;
    void Render() override;

    void LoadModel(const std::string& modelName, uint32_t modelUUID); 

private:

    // TODO Add model code, add mesh components when loading

    std::string currentModelName = "Not selected";
    uint32_t currentModelUUID = CONSTANT_NO_MODEL_UUID;
    
};
