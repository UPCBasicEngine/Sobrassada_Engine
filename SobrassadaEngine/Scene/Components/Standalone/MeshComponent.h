#pragma once
#include "Globals.h"
#include "Scene/Components/Component.h"

#include <cstdint>

class MeshComponent : public Component
{
public:
    MeshComponent(uint32_t uuid, uint32_t uuidParent, uint32_t uuidRoot, const char* name, const Transform& parentGlobalTransform);
    
    void RenderEditorInspector() override;
    void RenderEditorComponentTree(const uint32_t selectedComponentUUID) override;
    void Update() override;
    void Render() override;

    void LoadMesh(const std::string& meshName, uint32_t meshUUID); 
    
private:

    // TODO Add model code, add mesh components when loading

    std::string currentMeshName = "Not selected";
    uint32_t currentMeshUUID = CONSTANT_NO_MESH_UUID;

    std::string currentTextureName = "Not selected";
    uint32_t currentTexureUUID = CONSTANT_NO_TEXTURE_UUID;
    
};
