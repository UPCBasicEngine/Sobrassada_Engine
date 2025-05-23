#pragma once

#include "Component.h"

class ResourceMaterial;
class ResourceTexture;

class DecalComponent : public Component
{
  public:
    DecalComponent(UID uid, GameObject* parent);
    DecalComponent(const rapidjson::Value& initialState, GameObject* parent);
    ~DecalComponent() override;

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;

    void Update(float deltaTime) override;
    void Render(float deltaTime) override;
    void RenderDebug(float deltaTime) override;
    void RenderEditorInspector() override;
    void AddMaterial(UID resource);
    void ParentUpdated() override;

    const ResourceMaterial* GetResourceMaterial() const { return currentMaterial; }

  private:
    void RecalculateAABB();

  private:
    float width                       = 1;
    float height                      = 1;

    std::string currentResourceName   = "No material";

    UID currentMaterialUID            = DEFAULT_MATERIAL_UID;
    ResourceMaterial* currentMaterial = nullptr;
};