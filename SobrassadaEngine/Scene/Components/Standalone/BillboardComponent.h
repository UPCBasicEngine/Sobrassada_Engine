#pragma once

#include "Component.h"

class ResourceMaterial;

class BillboardComponent : public Component
{
  public:
    BillboardComponent(UID uid, GameObject* parent);
    BillboardComponent(const rapidjson::Value& initialState, GameObject* parent);
    ~BillboardComponent() override;

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;

    void Update(float deltaTime) override;
    void Render(float deltaTime) override;
    void RenderDebug(float deltaTime) override;
    void RenderEditorInspector() override;

  private:
    void CreateVertexBufferObject();
    void AddMaterial(UID resourceUID);

  private:
    unsigned int vbo                  = 0;

    float width                = 2.f;
    float height               = 2.f;
    bool lockPitch                    = false;

    std::string currentMaterialName   = "No material";
    ResourceMaterial* currentMaterial = nullptr;
};
