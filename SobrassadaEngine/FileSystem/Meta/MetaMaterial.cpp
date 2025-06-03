#include "MetaMaterial.h"

MetaMaterial::MetaMaterial(UID uid, const std::string& assetPath, const std::string& shader, bool useOcclusion, UID defaultTextureUID, bool isTransparent, bool isDoubleSided, bool isAlphaDiscard)
    : MetaFile(uid, assetPath), shader(shader), useOcclusion(useOcclusion), defaultTextureUID(defaultTextureUID),
      isTransparent(isTransparent), isDoubleSided(isDoubleSided), isAlphaDiscard(isAlphaDiscard)
{
}

void MetaMaterial::AddImportOptions(rapidjson::Document& doc, rapidjson::Document::AllocatorType& allocator) const
{
    rapidjson::Value importOptions(rapidjson::kObjectType);
    importOptions.AddMember("shader", rapidjson::Value(shader.c_str(), allocator), allocator);
    importOptions.AddMember("useOcclusion", useOcclusion, allocator);
    importOptions.AddMember("defaultTextureUID", defaultTextureUID, allocator);
    importOptions.AddMember("isAlphaDiscard", isAlphaDiscard, allocator);
    importOptions.AddMember("isTransparent", isTransparent, allocator);
    importOptions.AddMember("isDoubleSided", isDoubleSided, allocator);
    doc.AddMember("importOptions", importOptions, allocator);
}