#pragma once

#include "MetaFile.h"

class MetaAnimation : public MetaFile
{
  public:
    MetaAnimation(UID uid, const std::string& assetPath, int32_t index);
    void AddImportOptions(rapidjson::Document& doc, rapidjson::Document::AllocatorType& allocator) const override;

    private:
    int32_t animationIndex = -1;
};
