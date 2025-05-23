#pragma once

#include "HashString.h"
#include "Module.h"

#include <map>
#include <utility>
#include <vector>

class Billboard;
class ResourceMaterial;
class BillboardComponent;

class BillboardModule : public Module
{
  public:
    BillboardModule();
    ~BillboardModule() override;

    bool Init() override;
    bool ShutDown() override;

    update_status PostUpdate(float deltaTime) override;
    update_status Update(float deltaTime) override;

    void CreateTag(const char* newTag);
    void DeleteTag(const HashString tag);
    void RequestTag(const HashString& tag, BillboardComponent* component);
    void RemoveComponentFromTag(const HashString& tag, BillboardComponent* component);

    void RenderBillboards();

    void UpdateTagWidth(const HashString& tag, float width);
    void UpdateTagHeight(const HashString& tag, float height);
    void UpdateTagMaterial(const HashString& tag, UID material);
    void UpdateTagTexture(const HashString& tag, UID texture);
    void UpdateTagLockPitch(const HashString& tag, bool lockAxis);
    void UpdateTagUseTexture(const HashString& tag, bool useTexture);
    void UpdateTagPositions(const HashString& tag);
    void UpdateTagUVCoords(const HashString& tag, float xmin, float ymin, float selectionWidth, float selectionHeight);

    const std::vector<HashString>& GetTags() const { return billboardTags; };

  private:
    bool FindTag(const HashString& tag, int& outPosition);

  private:
    std::vector<HashString> billboardTags;
    std::map<HashString, std::pair<unsigned int, Billboard*>> billboardMap;

    HashString emptyString = HashString("");
};
