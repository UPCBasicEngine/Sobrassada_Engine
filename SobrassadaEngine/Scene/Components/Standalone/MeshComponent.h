#pragma once

#include "Globals.h"
#include "ResourceManagement/Resources/ResourceMaterial.h"
#include "ResourceManagement/Resources/ResourceMesh.h"
#include "Scene/Components/Component.h"

#include <cstdint>

class MeshComponent : public Component
{
public:
    MeshComponent(UID uid, UID uidParent, UID uidRoot, const char* name, const Transform& parentGlobalTransform);
    
    void RenderEditorInspector() override;
    void Update() override;
    void Render() override;

private:
    
    void AddMesh(UID resource);
    void AddMaterial(UID resource);

private:

    std::string currentMeshName = "Not selected";
    ResourceMesh* currentMesh = nullptr;

    std::string currentTextureName = "Not selected";
    ResourceMaterial* currentMaterial = nullptr;
    
};
