#pragma once

#include "Component.h"
#include "HashString.h"

#include <list>

constexpr const char* ResourceTypeStrings[] = {"Material", "Texture"};
constexpr int ResourceTypeStringsSize       = sizeof(ResourceTypeStrings) / sizeof(char*);

class ResourceMaterial;
class ResourceTexture;

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

    bool IsUsingTexture() const { return useTexture; }

    float GetWidth() const { return width; }
    float GetHeight() const { return height; }
    bool GetLockPitch() const { return lockPitch; }
    UID GetMaterialUID() const { return currentMaterialUID; }
    UID GetTextureUID() const { return currentTextureUID; }
    const HashString& GetBillboardTag() const { return billboardTag; }
    std::list<BillboardComponent*>::iterator GetBillboardIterator() const { return billboardIterator; }

    void SetWidth(float newWidth);
    void SetHeight(float newHeight);
    void SetMaterial(ResourceMaterial* newMaterial);
    void SetTexture(ResourceTexture* newTexture);
    void SetIterator(std::list<BillboardComponent*>::iterator iterator) { billboardIterator = iterator; };
    void SetLockPitch(bool newPitch) { lockPitch = newPitch; };
    void SetUseTexture(bool newUseTexture) { useTexture = newUseTexture; };

  private:
    void RecalculateAABB();

  private:
    float width                       = 1.f;
    float height                      = 1.f;
    bool lockPitch                    = false;

    char newTagName[64]               = "";
    HashString billboardTag           = HashString("");

    bool useTexture                   = false;
    std::string currentResourceName   = "No material";

    UID currentMaterialUID            = DEFAULT_MATERIAL_UID;
    ResourceMaterial* currentMaterial = nullptr;

    UID currentTextureUID             = FALLBACK_TEXTURE_UID;
    ResourceTexture* currentTexture   = nullptr;

    std::list<BillboardComponent*>::iterator billboardIterator;
};
