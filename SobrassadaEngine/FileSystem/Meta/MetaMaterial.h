#pragma once

#include "MetaFile.h"

class MetaMaterial : public MetaFile
{
  public:
    MetaMaterial(UID uid, const std::string& assetPath, const std::string& shader, bool useOcclusion, UID defaultTextureUID, bool isTransparent, bool isDoubleSided, bool isAlphaDiscard);
    void AddImportOptions(rapidjson::Document& doc, rapidjson::Document::AllocatorType& allocator) const override;

  private:
    std::string shader;
    bool useOcclusion;
    bool isAlphaDiscard;
    bool isTransparent;
    bool isDoubleSided;

    UID defaultTextureUID = INVALID_UID;
};