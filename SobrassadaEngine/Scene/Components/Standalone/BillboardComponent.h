#pragma once

#include "Component.h"
#include "HashString.h"

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

    float GetWidth() const { return width; }
    float GetHeight() const { return height; }

  private:

    float width                       = 1.f;
    float height                      = 1.f;
    bool lockPitch                    = false;

    int xTiles                        = 0;
    int yTiles                        = 0;
    float spriteSpeed                 = 0;

    char newTagName[64]               = "";
    HashString billboardTag           = HashString("");

    std::string currentMaterialName   = "No material";
    ResourceMaterial* currentMaterial = nullptr;
};
