#pragma once

#include "Component.h"
#include "HashString.h"

#include <list>

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
    void ParentUpdated() override;

    void ClearBillboardData();

    float GetWidth() const { return width; }
    float GetHeight() const { return height; }
    UID GetMaterialUID() const { return currentMaterialUID; }
    const HashString& GetBillboardTag() const { return billboardTag; }
    std::list<BillboardComponent*>::iterator GetBillboardIterator() const { return billboardIterator; }

    void SetWidth(float newWidth) { width = newWidth; };
    void SetHeight(float newHeight) { height = newHeight; };
    void SetMaterial(ResourceMaterial* newMaterial);
    void SetIterator(std::list<BillboardComponent*>::iterator iterator) { billboardIterator = iterator; };
    void SetLockPitch(bool newPitch) { lockPitch = newPitch; };

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
    UID currentMaterialUID            = DEFAULT_MATERIAL_UID;
    ResourceMaterial* currentMaterial = nullptr;

    std::list<BillboardComponent*>::iterator billboardIterator;
};
