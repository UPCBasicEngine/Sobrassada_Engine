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

    void RequestTag(const std::string& tag, BillboardComponent* component);

  private:
    std::vector<HashString> billboardTags;
    std::map<HashString, std::pair<unsigned int, Billboard*>> billboardMap;
};
