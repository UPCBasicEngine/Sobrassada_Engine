#pragma once
#include "EngineMesh.h"
#include "Globals.h"
#include "ResourceManagement/Resources/Resource.h"
#include "ResourceManagement/Resources/ResourceMaterial.h"
#include "ResourceManagement/Resources/ResourceMesh.h"
#include "Scene/Components/Component.h"

#include <cstdint>

class MeshComponent : public Component
{
public:
    MeshComponent(UID uid, UID uidParent, UID uidRoot, const char* name, const Transform& parentGlobalTransform);
    
    void RenderEditorInspector() override;
    void RenderEditorComponentTree(UID selectedComponentUID) override;
    void Update() override;
    void Render() override;

    
private:
    
    void AddMesh(Resource* resource);
    void AddMaterial(Resource* resource);

private:

    std::string currentMeshName = "Not selected";
    ResourceMesh* currentMesh = nullptr;

    std::string currentTextureName = "Not selected";
    ResourceMaterial* currentMaterial = nullptr;
    
};
