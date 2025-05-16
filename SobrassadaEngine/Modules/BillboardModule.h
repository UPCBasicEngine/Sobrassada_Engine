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

    void CreateTag(const char* newTag);
    void DeleteTag(const HashString& tag);
    void RequestTag(const HashString& tag, BillboardComponent* component);

    const std::vector<HashString>& GetTags() const { return billboardTags; };

  private:
    std::vector<HashString> billboardTags;
    std::map<HashString, std::pair<unsigned int, Billboard*>> billboardMap;
};
